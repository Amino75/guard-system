#include <stdio.h>

#include "common/config.h"
#include "common/logger.h"
#include "utils/utils.h"
#include "shared_memory/shared_memory.h"



int main(void)
{

    if(config_load("configs/settings.json") != 0)
    {
        printf("Config load failed\n");
        return 1;
    }



    if(log_init(
            config_get_string("logging.file"),
            LOG_DEBUG) != 0)
    {
        printf("Logger failed\n");
        return 1;
    }



    if(shm_init() != 0)
    {
        log_error(
            "Shared memory initialization failed"
        );

        return 1;
    }



    telemetry_t *data = shm_get();



    data->frame_number = 100;

    data->person_count = 2;

    data->fps = 30.5f;

    data->cpu_temperature =
        utils_get_cpu_temperature();



    data->guard_mode = true;

    data->person_detected = true;



    utils_get_timestamp(
        data->timestamp,
        sizeof(data->timestamp)
    );



    log_info(
        "Frame: %u",
        data->frame_number
    );


    log_info(
        "Persons: %u",
        data->person_count
    );


    log_info(
        "FPS: %.2f",
        data->fps
    );


    log_info(
        "Temperature: %.2f",
        data->cpu_temperature
    );


    log_info(
        "Time: %s",
        data->timestamp
    );



    shm_destroy();


    config_unload();

    log_close();



    return 0;
}