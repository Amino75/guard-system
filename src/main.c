#include <stdio.h>

#include "common/logger.h"


int main(void)
{

    if(log_init("logs/guard.log",
                LOG_DEBUG) != 0)
    {
        printf("Logger initialization failed\n");
        return 1;
    }


    log_info("Intelligent Guard System started");

    log_debug("Debug mode enabled");

    log_warning("Example warning message");

    log_error("Example error message");


    log_close();


    return 0;
}