#include <stdio.h>

#include "application/application.h"



int main(void)
{

    if(application_init() != 0)
    {
        printf(
            "Application initialization failed\n"
        );

        return 1;
    }



    application_run();



    application_shutdown();



    return 0;
}