#include <stdio.h>
#include <string.h>
#include "camera/camera.h"
#include "mongoose.h"
#include <stdlib.h>
#include "web/web_server.h"
#include "utils/utils.h"
#include "shared_memory/shared_memory.h"
#include <arpa/inet.h>
#include <time.h>

static struct mg_mgr mgr;
static uint64_t last_sent_frame_id = 0;

static int initialized = 0;

static char *read_file(const char *path)
{
    FILE *fp = fopen(path, "rb");

    if (fp == NULL)
    {
        return NULL;
    }

    fseek(fp, 0, SEEK_END);

    long size = ftell(fp);

    rewind(fp);

    if (size <= 0)
    {
        fclose(fp);
        return NULL;
    }

    char *buffer = malloc((size_t)size + 1);

    if (buffer == NULL)
    {
        fclose(fp);
        return NULL;
    }

    size_t read_size =
        fread(
            buffer,
            1,
            (size_t)size,
            fp
        );

    fclose(fp);

    if (read_size != (size_t)size)
    {
        free(buffer);
        return NULL;
    }

    buffer[size] = '\0';

    return buffer;
}

static char *ssl_cert = NULL;
static char *ssl_key = NULL;


/*
 * HTTP request handler
 */
static void http_handler(
    struct mg_connection *c,
    int ev,
    void *ev_data
)
{
    if (ev == MG_EV_ACCEPT)
{
    if (ntohs(c->loc.port) == 8443)
    {
        struct mg_tls_opts opts =
        {
            .cert = mg_str(ssl_cert),
            .key = mg_str(ssl_key)
        };

        mg_tls_init(c, &opts);
    }

    return;
}

    if(ev != MG_EV_HTTP_MSG)
        return;

    struct mg_http_message *hm =
        (struct mg_http_message *) ev_data;

if (ntohs(c->loc.port) == 8080)
{
    struct mg_str *host =
        mg_http_get_header(hm, "Host");

    if (host != NULL)
    {
        mg_printf(
            c,
            "HTTP/1.1 301 Moved Permanently\r\n"
            "Location: https://%.*s:8443%.*s\r\n"
            "Content-Length: 0\r\n"
            "\r\n",
            (int) host->len,
            host->buf,
            (int) hm->uri.len,
            hm->uri.buf
        );
    }
    else
    {
        mg_printf(
            c,
            "HTTP/1.1 301 Moved Permanently\r\n"
            "Location: https://127.0.0.1:8443%.*s\r\n"
            "Content-Length: 0\r\n"
            "\r\n",
            (int) hm->uri.len,
            hm->uri.buf
        );
    }

    return;
}


    /*
     * Dashboard
     */
    if(mg_match(
            hm->uri,
            mg_str("/"),
            NULL))
    {

        struct mg_http_serve_opts opts =
        {
            .root_dir = "html"
        };


        mg_http_serve_file(
            c,
            hm,
            "html/index.html",
            &opts
        );


        return;
    }



    /*
     * Latest image
     */
    if(mg_match(
            hm->uri,
            mg_str("/live/latest.jpg"),
            NULL))
    {

        struct mg_http_serve_opts opts =
        {
            .root_dir = "html"
        };


        mg_http_serve_file(
            c,
            hm,
            "html/live/latest.jpg",
            &opts
        );


        return;
    }



    /*
     * Telemetry endpoint
     */
    if(mg_match(
            hm->uri,
            mg_str("/telemetry"),
            NULL))
    {

        mg_http_reply(
            c,
            200,
            "Content-Type: application/json\r\n",
            "{"
            "\"temperature\":%.2f,"
            "\"cpu\":%.2f,"
            "\"memory\":%llu"
            "}",
            utils_get_cpu_temperature(),
            utils_get_cpu_usage(),
            (unsigned long long)
            utils_get_free_memory()
        );


        return;
    }



    /*
     * Person counter
     */
    if(mg_match(
            hm->uri,
            mg_str("/persons"),
            NULL))
    {

        telemetry_t *data = shm_get();

        int count = 0;


        if(data)
            count = data->person_count;



        mg_http_reply(
            c,
            200,
            "Content-Type: application/json\r\n",
            "{\"person_count\":%d}",
            count
        );


        return;
    }

if(mg_match(
        hm->uri,
        mg_str("/live/latest.jpg"),
        NULL))
{

    size_t size = 0;

    unsigned char *jpg =
        camera_get_jpeg(&size);


    if(jpg == NULL)
    {
        mg_http_reply(
            c,
            503,
            "",
            "No image\n"
        );

        return;
    }


    mg_printf(
        c,
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: image/jpeg\r\n"
        "Content-Length: %lu\r\n"
        "\r\n",
        size
    );


    mg_send(
        c,
        jpg,
        size
    );


    return;
}

if(mg_match(
        hm->uri,
        mg_str("/stream"),
        NULL))
{

    mg_printf(
        c,
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n"
        "Cache-Control: no-cache\r\n"
        "\r\n"
    );


    c->data[0] = 1;   // mark stream connection


    return;
}

    /*
     * 404
     */
    mg_http_reply(
        c,
        404,
        "",
        "Not found\n"
    );
}



/*
 * Start HTTP server
 */
int web_server_init(void)
{
    mg_mgr_init(&mgr);

    ssl_cert =
        read_file("cert/server.crt");

    ssl_key =
        read_file("cert/server.key");

    if (ssl_cert == NULL || ssl_key == NULL)
    {
        printf(
            "SSL certificate or key could not be loaded\n"
        );

        free(ssl_cert);
        free(ssl_key);

        ssl_cert = NULL;
        ssl_key = NULL;

        mg_mgr_free(&mgr);

        return -1;
    }

    printf(
        "SSL certificate and key loaded\n"
    );


    /*
     * HTTP server
     *
     * Port 8080 is used instead of port 80
     * so the service does not require a
     * privileged port.
     *
     * HTTP requests are redirected to HTTPS.
     */
    struct mg_connection *http_connection =
        mg_http_listen(
            &mgr,
            "http://0.0.0.0:8080",
            http_handler,
            NULL
        );


    if (http_connection == NULL)
    {
        printf(
            "HTTP server failed\n"
        );

        free(ssl_cert);
        free(ssl_key);

        ssl_cert = NULL;
        ssl_key = NULL;

        mg_mgr_free(&mgr);

        return -1;
    }


    /*
     * HTTPS server
     *
     * This is the main Guard System server.
     */
    struct mg_connection *https_connection =
        mg_http_listen(
            &mgr,
            "https://0.0.0.0:8443",
            http_handler,
            NULL
        );


    if (https_connection == NULL)
    {
        printf(
            "HTTPS server failed\n"
        );

        free(ssl_cert);
        free(ssl_key);

        ssl_cert = NULL;
        ssl_key = NULL;

        mg_mgr_free(&mgr);

        return -1;
    }


    initialized = 1;


    printf(
        "HTTP server started on port 8080\n"
    );

    printf(
        "HTTPS server started on port 8443\n"
    );


    return 0;
}

void web_server_poll(void)
{
    if (!initialized)
        return;

    mg_mgr_poll(
        &mgr,
        10
    );

    struct mg_connection *c;

    for (c = mgr.conns; c != NULL; c = c->next)
    {
        if (c->data[0] != 1)
            continue;

        /*
         * Do not build a backlog of old frames.
         */
        if (c->send.len > 100000)
            continue;

        /*
         * Get the newest camera frame ID.
         */
        uint64_t frame_id =
            camera_get_frame_id();

        /*
         * Nothing new to send.
         */
        if (frame_id == last_sent_frame_id)
            continue;

        size_t size = 0;

        unsigned char *jpg =
            camera_get_jpeg(&size);

        if (jpg == NULL || size == 0)
            continue;

        /*
         * Send the newest JPEG frame.
         */
        mg_printf(
            c,
            "--frame\r\n"
            "Content-Type: image/jpeg\r\n"
            "Content-Length: %lu\r\n"
            "\r\n",
            size
        );

        mg_send(
            c,
            jpg,
            size
        );

        mg_printf(
            c,
            "\r\n"
        );

        /*
         * Mark this camera frame as sent.
         */
        last_sent_frame_id = frame_id;
    }
}

void web_server_stop(void)
{

    if(!initialized)
        return;


    mg_mgr_free(
        &mgr
    );


    initialized = 0;


    printf(
        "HTTP server stopped\n"
    );

}
