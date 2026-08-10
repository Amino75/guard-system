#include "application/application.h"
#include "history/history.h"
#include "camera/camera.h"
#include "email/email.h"
#include <signal.h>
#include <stdio.h>
#include <time.h>


#include "mqtt/mqtt.h"
#include "common/config.h"
#include "common/logger.h"
#include "shared_memory/shared_memory.h"
#include "utils/utils.h"
#include "web/web_server.h"

static volatile sig_atomic_t running = 1;


static void signal_handler(int signal)
{
    (void)signal;

    running = 0;
}


int application_init(void)
{
    /*
     * Load configuration.
     */

    if (config_load("configs/settings.json") != 0)
    {
        printf(
            "Configuration loading failed\n"
        );

        return -1;
    }


    /*
     * Initialize logger.
     */

    const char *log_file =
        config_get_string("logging.file");


    if (log_file == NULL)
    {
        printf(
            "Log file missing\n"
        );

        return -1;
    }


    if (log_init(
        log_file,
        LOG_DEBUG
    ) != 0)
    {
        printf(
            "Logger initialization failed\n"
        );

        return -1;
    }

    /*
     * Initialize web server.
     */

    if (web_server_init() != 0)
    {
        log_error(
            "Web server initialization failed"
        );

        return -1;
    }


    log_info(
        "Application initialization"
    );


    /*
     * Initialize shared memory.
     */

    if (shm_init() != 0)
    {
        log_error(
            "Shared memory initialization failed"
        );

        return -1;
    }

/*
 * Initialize history.
 */

if (history_init() != 0)
{
    log_error(
        "History initialization failed"
    );

    return -1;
}

    /*
     * Register signal handlers.
     */

    signal(
        SIGINT,
        signal_handler
    );

    signal(
        SIGTERM,
        signal_handler
    );


/*
 * Initialize email subsystem.
 */

if (email_init() != 0)
{
    log_error(
        "Email initialization failed"
    );

    return -1;
}


/*
 * Initialize MQTT.
 */

if (mqtt_init() != 0)
{
    log_error(
        "MQTT initialization failed"
    );

    return -1;
}

    /*
     * Initialize camera.
     */

    if (camera_init() != 0)
    {
        log_error(
            "Camera initialization failed"
        );

        return -1;
    }


    log_info(
        "Initialization complete"
    );


    return 0;
}


void application_run(void)
{
    log_info(
        "Application main loop started"
    );


    /*
     * Telemetry update timer.
     */
    time_t last_telemetry =
        time(NULL);


    while (running)
    {
        /*
         * Capture camera frame.
         *
         * This runs continuously.
         */
        camera_capture();


        /*
         * Process network requests.
         */
        web_server_poll();


        /*
         * Update telemetry approximately
         * once per second.
         */
        time_t now =
            time(NULL);


        if (
            now != last_telemetry
        )
        {
            last_telemetry =
                now;


            telemetry_t *data =
                shm_get();


if (data != NULL)
{
    data->cpu_temperature =
        utils_get_cpu_temperature();


    utils_get_timestamp(
        data->timestamp,
        sizeof(data->timestamp)
    );

    /*
     * Publish current system state to MQTT.
     *
     * Both MQTT topics use the same current
     * person count, CPU temperature and timestamp.
     */
    if (mqtt_is_connected())
    {
        if (
            mqtt_publish_persons(
                data->person_count,
                data->timestamp,
                data->cpu_temperature
            ) != 0
        )
        {
            log_error(
                "MQTT persons publish failed"
            );
        }

        if (
            mqtt_publish_telemetry(
                data->person_count,
                data->timestamp,
                data->cpu_temperature
            ) != 0
        )
        {
            log_error(
                "MQTT telemetry publish failed"
            );
        }
    }


    /*
     * Add current telemetry to history.
     */
    if (
        history_add(
            data->person_count,
            data->cpu_temperature
        ) != 0
    )
    {
        log_error(
            "Failed to add history entry"
        );
    }
}


            log_debug(
                "System heartbeat"
            );
        }
    }


    log_info(
        "Main loop stopped"
    );
}


void application_shutdown(void)
{
    log_info(
        "Application shutdown"
    );


    camera_stop();
mqtt_cleanup();

    web_server_stop();
email_cleanup();
history_shutdown();
    shm_destroy();

email_cleanup();
    config_unload();


    log_close();
}
