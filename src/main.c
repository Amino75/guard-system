#include <stdio.h>

#include "common/logger.h"
#include "common/config.h"

int main(void)
{
    if(config_load("configs/settings.json") != 0)
    {
        printf("Cannot load configuration\n");
        return 1;
    }

    if(log_init(config_get_string("logging.file"),
                LOG_DEBUG) != 0)
    {
        printf("Logger initialization failed\n");
        return 1;
    }

    log_info("Configuration loaded");

    log_info("Application : %s",
             config_get_string("application.name"));

    log_info("Version : %s",
             config_get_string("application.version"));

    log_info("MQTT Host : %s",
             config_get_string("mqtt.host"));

    log_info("MQTT Port : %d",
             config_get_int("mqtt.port"));

    log_info("HTTPS Port : %d",
             config_get_int("webserver.port"));

    log_info("Camera : %d",
             config_get_int("camera.device"));

    config_unload();

    log_close();

    return 0;
}