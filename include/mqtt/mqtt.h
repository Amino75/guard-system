#ifndef MQTT_H
#define MQTT_H

#include <stdbool.h>

int mqtt_init(void);

int mqtt_publish_persons(
    unsigned int person_count,
    const char *timestamp,
    double cpu_temperature
);

int mqtt_publish_telemetry(
    unsigned int person_count,
    const char *timestamp,
    double cpu_temperature
);

bool mqtt_is_connected(void);

void mqtt_cleanup(void);

#endif
