#include "mqtt/mqtt.h"
#include <stdlib.h>
#include <mosquitto.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cJSON.h"

#define MQTT_HOST "192.168.1.100"
#define MQTT_PORT 1883

#define MQTT_CLIENT_ID "guard-system-404211106"

#define MQTT_PERSONS_TOPIC \
    "persons/404211106/home"

#define MQTT_TELEMETRY_TOPIC \
    "telemetry/404211106/home"

#define MQTT_STATUS_TOPIC \
    "status/404211106/home"

#define MQTT_QOS 1

static struct mosquitto *mqtt_client = NULL;

static bool mqtt_connected = false;


/*
 * MQTT connection callback.
 */
static void mqtt_on_connect(
    struct mosquitto *mosq,
    void *userdata,
    int rc
)
{
    (void)mosq;
    (void)userdata;

    if (rc == 0)
    {
        mqtt_connected = true;

        printf(
            "MQTT connected to %s:%d\n",
            MQTT_HOST,
            MQTT_PORT
        );

        /*
         * Publish online status.
         *
         * The broker will publish the LWT offline
         * message automatically if this client
         * disappears unexpectedly.
         */
        const char *online_message =
            "{\"status\":\"online\"}";

        int result =
            mosquitto_publish(
                mqtt_client,
                NULL,
                MQTT_STATUS_TOPIC,
                (int)strlen(online_message),
                online_message,
                MQTT_QOS,
                true
            );

        if (result != MOSQ_ERR_SUCCESS)
        {
            fprintf(
                stderr,
                "MQTT online status publish failed: %s\n",
                mosquitto_strerror(result)
            );
        }
    }
    else
    {
        mqtt_connected = false;

        fprintf(
            stderr,
            "MQTT connection failed: %s\n",
            mosquitto_connack_string(rc)
        );
    }
}


/*
 * MQTT disconnect callback.
 */
static void mqtt_on_disconnect(
    struct mosquitto *mosq,
    void *userdata,
    int rc
)
{
    (void)mosq;
    (void)userdata;

    mqtt_connected = false;

    if (rc == 0)
    {
        printf(
            "MQTT disconnected normally\n"
        );
    }
    else
    {
        fprintf(
            stderr,
            "MQTT disconnected unexpectedly: %s\n",
            mosquitto_strerror(rc)
        );
    }
}


/*
 * Build and publish a JSON message.
 */
static int mqtt_publish_json(
    const char *topic,
    unsigned int person_count,
    const char *timestamp,
    double cpu_temperature
)
{
    if (!mqtt_connected)
    {
        return -1;
    }

    if (topic == NULL || timestamp == NULL)
    {
        return -1;
    }

    cJSON *root =
        cJSON_CreateObject();

    if (root == NULL)
    {
        return -1;
    }

    cJSON_AddNumberToObject(
        root,
        "person_count",
        (double)person_count
    );

    cJSON_AddNumberToObject(
        root,
        "temperature",
        cpu_temperature
    );

    cJSON_AddStringToObject(
        root,
        "timestamp",
        timestamp
    );

    char *json =
        cJSON_PrintUnformatted(root);

    if (json == NULL)
    {
        cJSON_Delete(root);

        return -1;
    }

    int result =
        mosquitto_publish(
            mqtt_client,
            NULL,
            topic,
            (int)strlen(json),
            json,
            MQTT_QOS,
            false
        );

    if (result != MOSQ_ERR_SUCCESS)
    {
        fprintf(
            stderr,
            "MQTT publish failed for %s: %s\n",
            topic,
            mosquitto_strerror(result)
        );

        free(json);
        cJSON_Delete(root);

        return -1;
    }

    printf(
        "MQTT published: %s -> %s\n",
        topic,
        json
    );

    free(json);
    cJSON_Delete(root);

    return 0;
}


/*
 * Initialize MQTT client.
 */
int mqtt_init(void)
{
    int result;


    /*
     * Initialize libmosquitto.
     */
    result =
        mosquitto_lib_init();

    if (result != MOSQ_ERR_SUCCESS)
    {
        fprintf(
            stderr,
            "mosquitto_lib_init failed: %s\n",
            mosquitto_strerror(result)
        );

        return -1;
    }


    /*
     * Create MQTT client.
     *
     * MQTT v3.1.1 is used because the installed
     * libmosquitto version supports it reliably.
     */
    mqtt_client =
        mosquitto_new(
            MQTT_CLIENT_ID,
            true,
            NULL
        );

    if (mqtt_client == NULL)
    {
        fprintf(
            stderr,
            "mosquitto_new failed\n"
        );

        mosquitto_lib_cleanup();

        return -1;
    }


    /*
     * Register callbacks.
     */
    mosquitto_connect_callback_set(
        mqtt_client,
        mqtt_on_connect
    );

    mosquitto_disconnect_callback_set(
        mqtt_client,
        mqtt_on_disconnect
    );


    /*
     * Configure Last Will and Testament.
     *
     * If the board loses power, crashes, or the
     * MQTT connection disappears unexpectedly,
     * Mosquitto will publish this retained message.
     */
    const char *offline_message =
        "{\"status\":\"offline\"}";

    result =
        mosquitto_will_set(
            mqtt_client,
            MQTT_STATUS_TOPIC,
            (int)strlen(offline_message),
            offline_message,
            MQTT_QOS,
            true
        );

    if (result != MOSQ_ERR_SUCCESS)
    {
        fprintf(
            stderr,
            "mosquitto_will_set failed: %s\n",
            mosquitto_strerror(result)
        );

        mosquitto_destroy(mqtt_client);
        mqtt_client = NULL;

        mosquitto_lib_cleanup();

        return -1;
    }


    /*
     * Connect to the broker.
     */
    result =
        mosquitto_connect(
            mqtt_client,
            MQTT_HOST,
            MQTT_PORT,
            60
        );

    if (result != MOSQ_ERR_SUCCESS)
    {
        fprintf(
            stderr,
            "MQTT connection failed: %s\n",
            mosquitto_strerror(result)
        );

        mosquitto_destroy(mqtt_client);
        mqtt_client = NULL;

        mosquitto_lib_cleanup();

        return -1;
    }


    /*
     * Start the MQTT network loop in its own thread.
     *
     * This prevents MQTT network activity from
     * blocking the camera capture loop.
     */
    result =
        mosquitto_loop_start(
            mqtt_client
        );

    if (result != MOSQ_ERR_SUCCESS)
    {
        fprintf(
            stderr,
            "mosquitto_loop_start failed: %s\n",
            mosquitto_strerror(result)
        );

        mosquitto_disconnect(
            mqtt_client
        );

        mosquitto_destroy(
            mqtt_client
        );

        mqtt_client = NULL;

        mosquitto_lib_cleanup();

        return -1;
    }


    return 0;
}


/*
 * Publish person detection data.
 */
int mqtt_publish_persons(
    unsigned int person_count,
    const char *timestamp,
    double cpu_temperature
)
{
    return mqtt_publish_json(
        MQTT_PERSONS_TOPIC,
        person_count,
        timestamp,
        cpu_temperature
    );
}


/*
 * Publish telemetry data.
 */
int mqtt_publish_telemetry(
    unsigned int person_count,
    const char *timestamp,
    double cpu_temperature
)
{
    return mqtt_publish_json(
        MQTT_TELEMETRY_TOPIC,
        person_count,
        timestamp,
        cpu_temperature
    );
}


/*
 * Return MQTT connection state.
 */
bool mqtt_is_connected(void)
{
    return mqtt_connected;
}


/*
 * Cleanup MQTT.
 */
void mqtt_cleanup(void)
{
    if (mqtt_client != NULL)
    {
        mosquitto_loop_stop(
            mqtt_client,
            true
        );

        mosquitto_disconnect(
            mqtt_client
        );

        mosquitto_destroy(
            mqtt_client
        );

        mqtt_client = NULL;
    }

    mqtt_connected = false;

    mosquitto_lib_cleanup();
}
