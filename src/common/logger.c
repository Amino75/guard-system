#include "common/logger.h"

#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <pthread.h>


static FILE *log_file = NULL;

static log_level_t current_level = LOG_DEBUG;

static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;



static const char *level_string(log_level_t level)
{
    switch(level)
    {
        case LOG_DEBUG:
            return "DEBUG";

        case LOG_INFO:
            return "INFO";

        case LOG_WARNING:
            return "WARNING";

        case LOG_ERROR:
            return "ERROR";

        default:
            return "UNKNOWN";
    }
}



int log_init(const char *filename, log_level_t level)
{
    current_level = level;

    log_file = fopen(filename, "a");

    if(log_file == NULL)
    {
        return -1;
    }


    return 0;
}



void log_close(void)
{
    pthread_mutex_lock(&log_mutex);


    if(log_file)
    {
        fclose(log_file);
        log_file = NULL;
    }


    pthread_mutex_unlock(&log_mutex);
}



static void log_write(log_level_t level,
                      const char *format,
                      va_list args)
{

    if(level < current_level)
    {
        return;
    }


    pthread_mutex_lock(&log_mutex);


    time_t now;
    time(&now);


    struct tm time_info;

    localtime_r(&now, &time_info);


    char timestamp[64];

    strftime(timestamp,
             sizeof(timestamp),
             "%Y-%m-%d %H:%M:%S",
             &time_info);



    printf("[%s] [%s] ",
           timestamp,
           level_string(level));


    vprintf(format,args);

    printf("\n");



    if(log_file)
    {
        fprintf(log_file,
                "[%s] [%s] ",
                timestamp,
                level_string(level));


        va_list copy;
        va_copy(copy,args);


        vfprintf(log_file,
                 format,
                 copy);


        va_end(copy);


        fprintf(log_file,"\n");

        fflush(log_file);
    }


    pthread_mutex_unlock(&log_mutex);

}



void log_debug(const char *format,...)
{
    va_list args;

    va_start(args,format);

    log_write(LOG_DEBUG,format,args);

    va_end(args);
}



void log_info(const char *format,...)
{
    va_list args;

    va_start(args,format);

    log_write(LOG_INFO,format,args);

    va_end(args);
}



void log_warning(const char *format,...)
{
    va_list args;

    va_start(args,format);

    log_write(LOG_WARNING,format,args);

    va_end(args);
}



void log_error(const char *format,...)
{
    va_list args;

    va_start(args,format);

    log_write(LOG_ERROR,format,args);

    va_end(args);
}