#include <stdio.h>
#include <string.h>
#include "camera/camera.h"
#include "mongoose.h"

#include "web/web_server.h"
#include "utils/utils.h"
#include "shared_memory/shared_memory.h"


static struct mg_mgr mgr;

static int initialized = 0;


/*
 * HTTP request handler
 */
static void http_handler(
    struct mg_connection *c,
    int ev,
    void *ev_data
)
{
    if(ev != MG_EV_HTTP_MSG)
        return;


    struct mg_http_message *hm =
        (struct mg_http_message *) ev_data;



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

    mg_mgr_init(
        &mgr
    );


    struct mg_connection *c =
        mg_http_listen(
            &mgr,
            "http://0.0.0.0:80",
            http_handler,
            NULL
        );


    if(c == NULL)
    {
        printf(
            "HTTP server failed\n"
        );

        return -1;
    }



    initialized = 1;


    printf(
        "HTTP server started on port 80\n"
    );


    return 0;
}


void web_server_poll(void)
{

    if(!initialized)
        return;


    mg_mgr_poll(
        &mgr,
        10
    );


    struct mg_connection *c;


    for(c = mgr.conns; c != NULL; c = c->next)
    {

        if(c->data[0] == 1)
        {

            size_t size = 0;


            unsigned char *jpg =
                camera_get_jpeg(&size);


            if(jpg && size)
            {

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
            }
        }
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
