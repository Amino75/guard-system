#include "application/application.h"

#include <signal.h>
#include <stdio.h>

#include "common/config.h"
#include "common/logger.h"
#include "shared_memory/shared_memory.h"
#include "utils/utils.h"



static volatile sig_atomic_t running = 1;



static void signal_handler(int signal)
{
    (void)signal;

    running = 0;
}



int application_init(void)
{

    /*
     * Load configuration
     */

    if(config_load("configs/settings.json") != 0)
    {
        printf("Configuration loading failed\n");
        return -1;
    }



    /*
     * Initialize logger
     */

    const char *log_file =
        config_get_string("logging.file");


    if(log_file == NULL)
    {
        printf("Log file missing\n");
        return -1;
    }



    if(log_init(log_file, LOG_DEBUG) != 0)
    {
        printf("Logger initialization failed\n");
        return -1;
    }



    log_info("Application initialization");



    /*
     * Initialize shared memory
     */

    if(shm_init() != 0)
    {
        log_error(
            "Shared memory initialization failed"
        );

        return -1;
    }



    /*
     * Register Ctrl+C handler
     */

    signal(
        SIGINT,
        signal_handler
    );


    signal(
        SIGTERM,
        signal_handler
    );



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


    while(running)
    {

        telemetry_t *data =
            shm_get();


        if(data)
        {

            data->cpu_temperature =
                utils_get_cpu_temperature();


            utils_get_timestamp(
                data->timestamp,
                sizeof(data->timestamp)
            );

        }



        log_debug(
            "System heartbeat"
        );


        utils_sleep_ms(1000);

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


    shm_destroy();


    config_unload();


    log_close();

}