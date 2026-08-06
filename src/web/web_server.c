#include <stdio.h>

#include "mongoose.h"
#include "web/web_server.h"


static struct mg_mgr mgr;
static int initialized = 0;



static void http_handler(
    struct mg_connection *c,
    int ev,
    void *ev_data,
    void *fn_data
)
{
    (void)fn_data;


    if(ev == MG_EV_HTTP_MSG)
    {
        struct mg_http_message *hm =
            (struct mg_http_message *) ev_data;


        if(mg_match(
                hm->uri,
                mg_str("/"),
                NULL))
        {
            mg_http_reply(
                c,
                200,
                "Content-Type: text/plain\r\n",
                "Hello from Guard System\n"
            );
        }
        else
        {
            mg_http_reply(
                c,
                404,
                "",
                "Not Found\n"
            );
        }
    }
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


    if(connection == NULL)
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
    if(!initialized)
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

    if(!initialized)
    {
        return;
    }


    mg_mgr_free(&mgr);

    initialized = 0;


    printf(
        "Web server stopped\n"
    );
}