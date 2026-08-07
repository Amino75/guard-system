#include <stdio.h>
#include "utils/utils.h"
#include "mongoose.h"
#include "web/web_server.h"
#include "shared_memory/shared_memory.h"
#include <stdlib.h>


static struct mg_mgr mgr;
static int initialized = 0;
static char stream_path[] = "html/live/latest.jpg";
#define STREAM_FLAG 1

static void send_jpeg_frame(struct mg_connection *c)
{
    FILE *fp = fopen(stream_path, "rb");

    if (fp == NULL)
    {
        return;
    }


    fseek(fp, 0, SEEK_END);

    long size = ftell(fp);

    rewind(fp);


    if(size <= 0)
    {
        fclose(fp);
        return;
    }


    unsigned char *buffer =
        malloc(size);


    if(buffer == NULL)
    {
        fclose(fp);
        return;
    }


    fread(
        buffer,
        1,
        size,
        fp
    );

    fclose(fp);



    mg_printf(
        c,
        "--frame\r\n"
        "Content-Type: image/jpeg\r\n"
        "Content-Length: %ld\r\n"
        "\r\n",
        size
    );


    mg_send(
        c,
        buffer,
        size
    );


    mg_printf(
        c,
        "\r\n"
    );


    free(buffer);
}


static void http_handler(
    struct mg_connection *c,
    int ev,
    void *ev_data
)
{

if (ev == MG_EV_POLL)
{
    if (c->data[0] == STREAM_FLAG)
    {
        send_jpeg_frame(c);
    }

    return;
}

    if (ev != MG_EV_HTTP_MSG)
    {
        return;
    }

    struct mg_http_message *hm =
        (struct mg_http_message *) ev_data;
/*
 * Telemetry API
 */
if (mg_match(hm->uri, mg_str("/telemetry"), NULL))
{
    float cpu_temp =
        utils_get_cpu_temperature();

    uint64_t free_memory =
        utils_get_free_memory();

    float cpu_usage =
        utils_get_cpu_usage();


    mg_http_reply(
        c,
        200,
        "Content-Type: application/json\r\n",
        "{"
        "\"cpu_temp\":%.2f,"
        "\"free_memory\":%llu,"
        "\"cpu_usage\":%.2f"
        "}",
        cpu_temp,
        (unsigned long long) free_memory,
        cpu_usage
    );

    return;
}

/*
 * Person count API
 */
if (mg_match(hm->uri, mg_str("/persons"), NULL))
{
    telemetry_t *data = shm_get();

    uint32_t count = 0;

    if (data != NULL)
    {
        count = data->person_count;
    }


    mg_http_reply(
        c,
        200,
        "Content-Type: application/json\r\n",
        "{"
        "\"person_count\":%u"
        "}",
        count
    );

    return;
}

/*
 * MJPEG stream
 */

if (mg_match(hm->uri, mg_str("/stream"), NULL))
{
    mg_printf(
        c,
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n"
        "Cache-Control: no-cache\r\n"
        "\r\n"
    );

    c->data[0] = STREAM_FLAG;

    return;
}

    /*
     * Serve the main dashboard.
     */
    if (mg_match(hm->uri, mg_str("/live/latest.jpg"), NULL))
    {
        struct mg_http_serve_opts opts = {0};

        opts.root_dir = "./html";

        mg_http_serve_dir(
            c,
            hm,
            &opts
        );

        return;
    }


    /*
     * Unknown URL.
     */
    mg_http_reply(
        c,
        404,
        "Content-Type: text/plain\r\n",
        "Not Found\n"
    );
}


int web_server_init(void)
{
    mg_mgr_init(&mgr);


    struct mg_connection *connection =
        mg_http_listen(
            &mgr,
            "http://0.0.0.0:8000",
            http_handler,
            NULL
        );


    if (connection == NULL)
    {
        printf(
            "Web server initialization failed\n"
        );

        mg_mgr_free(&mgr);

        return -1;
    }


    initialized = 1;


    printf(
        "Web server initialized on port 8000\n"
    );


    return 0;
}


void web_server_poll(void)
{
    if (!initialized)
    {
        return;
    }


    mg_mgr_poll(
        &mgr,
        0
    );
}


void web_server_stop(void)
{
    if (!initialized)
    {
        return;
    }


    mg_mgr_free(&mgr);

    initialized = 0;


    printf(
        "Web server stopped\n"
    );
}
