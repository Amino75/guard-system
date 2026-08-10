#include "mqtt/mqtt.h"
#include <stdlib.h>
#include <mosquitto.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common/config.h"
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
 *
 * MQTT is optional. Failure to connect to the broker
 * must never prevent the Guard System from starting.
 *
 * The network loop runs in a background thread and
 * automatically handles reconnect attempts.
 */
int mqtt_init(void)
{
    int result;
const char *mqtt_host =
    config_get_string("mqtt.host");

int mqtt_port =
    config_get_int("mqtt.port");

const char *mqtt_username =
    config_get_string("mqtt.username");

const char *mqtt_password =
    config_get_string("mqtt.password");
    /*
     * Initialize libmosquitto.
     */
    result = mosquitto_lib_init();

    if (result != MOSQ_ERR_SUCCESS)
    {
        fprintf(
            stderr,
            "mosquitto_lib_init failed: %s\n",
            mosquitto_strerror(result)
        );

        /*
         * MQTT is optional.
         * Do not abort the application.
         */
        return 0;
    }

    /*
     * Create MQTT client.
     */
    mqtt_client =
        mosquitto_new(
            MQTT_CLIENT_ID,
            true,
            NULL
        );
result =
    mosquitto_username_pw_set(
        mqtt_client,
        mqtt_username,
        mqtt_password
    );


    if (mqtt_client == NULL)
    {
        fprintf(
            stderr,
            "mosquitto_new failed\n"
        );

        mosquitto_lib_cleanup();

        /*
         * MQTT is optional.
         */
        return 0;
    }
result =
    mosquitto_username_pw_set(
        mqtt_client,
        mqtt_username,
        mqtt_password
    );

if (result != MOSQ_ERR_SUCCESS)
{
    fprintf(
        stderr,
        "MQTT authentication setup failed: %s\n",
        mosquitto_strerror(result)
    );

    mosquitto_destroy(mqtt_client);
    mqtt_client = NULL;

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

        /*
         * MQTT is optional.
         */
        return 0;
    }

    /*
     * Start MQTT network loop FIRST.
     *
     * This creates the background network thread.
     * It also allows Mosquitto to perform automatic
     * reconnect attempts without blocking the main
     * application.
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

        mosquitto_destroy(mqtt_client);
        mqtt_client = NULL;

        mosquitto_lib_cleanup();

        /*
         * MQTT is optional.
         */
        return 0;
    }

    /*
     * Request connection asynchronously.
     *
     * IMPORTANT:
     * Do NOT use mosquitto_connect() here.
     *
     * mosquitto_connect() can block while waiting for
     * an unreachable broker.
     *
     * mosquitto_connect_async() returns immediately.
     */
    result =
        mosquitto_connect_async(
            mqtt_client,
            mqtt_host,
            mqtt_port,
            60
        );

    if (result != MOSQ_ERR_SUCCESS)
    {
        fprintf(
            stderr,
            "MQTT asynchronous connection failed: %s\n",
            mosquitto_strerror(result)
        );

        /*
         * Keep the MQTT client alive.
         *
         * The background loop can continue handling
         * reconnect attempts.
         */
        mqtt_connected = false;

        return 0;
    }

    /*
     * MQTT initialization succeeded.
     *
     * The actual broker connection may happen later
     * through mqtt_on_connect().
     */
    printf(
        "MQTT subsystem initialized "
        "(broker %s:%d)\n",
        MQTT_HOST,
        MQTT_PORT
    );

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
