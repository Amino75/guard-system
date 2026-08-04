#include "utils/utils.h"

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdint.h>


void utils_get_timestamp(char *buffer, size_t size)
{
    if(buffer == NULL || size == 0)
        return;


    time_t now = time(NULL);

    struct tm time_info;

    localtime_r(&now, &time_info);


    strftime(buffer,
             size,
             "%Y-%m-%d %H:%M:%S",
             &time_info);
}



uint64_t utils_get_time_ms(void)
{
    struct timespec ts;


    clock_gettime(CLOCK_MONOTONIC, &ts);


    return ((uint64_t)ts.tv_sec * 1000)
           +
           (ts.tv_nsec / 1000000);
}



void utils_sleep_ms(unsigned int ms)
{
    usleep(ms * 1000);
}



bool utils_file_exists(const char *path)
{
    if(path == NULL)
        return false;


    FILE *file = fopen(path, "r");


    if(file)
    {
        fclose(file);
        return true;
    }


    return false;
}



int utils_create_directory(const char *path)
{
    if(path == NULL)
        return -1;


    struct stat st;


    if(stat(path, &st) == 0)
    {
        return 0;
    }


    return mkdir(path, 0755);
}



void utils_str_copy(char *dest,
                    const char *src,
                    size_t size)
{
    if(dest == NULL || src == NULL || size == 0)
        return;


    strncpy(dest,
            src,
            size - 1);


    dest[size - 1] = '\0';
}



float utils_get_cpu_temperature(void)
{
    FILE *fp =
        fopen("/sys/class/thermal/thermal_zone0/temp",
              "r");


    if(fp == NULL)
        return -1.0f;


    int temp;


    fscanf(fp,"%d",&temp);


    fclose(fp);


    return temp / 1000.0f;
}