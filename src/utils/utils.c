#include "utils/utils.h"

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>


void utils_get_timestamp(
    char *buffer,
    size_t size
)
{
    if (buffer == NULL || size == 0)
    {
        return;
    }

    time_t now = time(NULL);

    struct tm time_info;

    localtime_r(
        &now,
        &time_info
    );

    strftime(
        buffer,
        size,
        "%Y-%m-%d %H:%M:%S",
        &time_info
    );
}


uint64_t utils_get_time_ms(void)
{
    struct timespec ts;

    clock_gettime(
        CLOCK_MONOTONIC,
        &ts
    );

    return
        ((uint64_t) ts.tv_sec * 1000)
        +
        (ts.tv_nsec / 1000000);
}


void utils_sleep_ms(unsigned int ms)
{
    usleep(ms * 1000);
}


bool utils_file_exists(
    const char *path
)
{
    if (path == NULL)
    {
        return false;
    }

    FILE *file = fopen(
        path,
        "r"
    );

    if (file != NULL)
    {
        fclose(file);
        return true;
    }

    return false;
}


int utils_create_directory(
    const char *path
)
{
    if (path == NULL)
    {
        return -1;
    }

    struct stat st;

    if (stat(path, &st) == 0)
    {
        return 0;
    }

    return mkdir(
        path,
        0755
    );
}


void utils_str_copy(
    char *dest,
    const char *src,
    size_t size
)
{
    if (
        dest == NULL ||
        src == NULL ||
        size == 0
    )
    {
        return;
    }

    strncpy(
        dest,
        src,
        size - 1
    );

    dest[size - 1] = '\0';
}


float utils_get_cpu_temperature(void)
{
    FILE *fp =
        fopen(
            "/sys/class/thermal/thermal_zone0/temp",
            "r"
        );

    if (fp == NULL)
    {
        return -1.0f;
    }

    int temp = 0;

    if (fscanf(fp, "%d", &temp) != 1)
    {
        fclose(fp);
        return -1.0f;
    }

    fclose(fp);

    return temp / 1000.0f;
}


uint64_t utils_get_free_memory(void)
{
    FILE *fp =
        fopen(
            "/proc/meminfo",
            "r"
        );

    if (fp == NULL)
    {
        return 0;
    }

    char line[256];

    uint64_t mem_available_kb = 0;

    while (fgets(
        line,
        sizeof(line),
        fp
    ) != NULL)
    {
        if (
            sscanf(
                line,
                "MemAvailable: %lu kB",
                &mem_available_kb
            ) == 1
        )
        {
            break;
        }
    }

    fclose(fp);

    return mem_available_kb * 1024ULL;
}


float utils_get_cpu_usage(void)
{
    static uint64_t previous_total = 0;
    static uint64_t previous_idle = 0;

    FILE *fp =
        fopen(
            "/proc/stat",
            "r"
        );

    if (fp == NULL)
    {
        return -1.0f;
    }

    char line[256];

    uint64_t user = 0;
    uint64_t nice = 0;
    uint64_t system = 0;
    uint64_t idle = 0;
    uint64_t iowait = 0;
    uint64_t irq = 0;
    uint64_t softirq = 0;
    uint64_t steal = 0;

    if (
        fgets(
            line,
            sizeof(line),
            fp
        ) == NULL
    )
    {
        fclose(fp);
        return -1.0f;
    }

    fclose(fp);

    int fields =
        sscanf(
            line,
            "cpu %lu %lu %lu %lu %lu %lu %lu %lu",
            &user,
            &nice,
            &system,
            &idle,
            &iowait,
            &irq,
            &softirq,
            &steal
        );

    if (fields < 4)
    {
        return -1.0f;
    }

    uint64_t idle_total =
        idle + iowait;

    uint64_t total =
        user +
        nice +
        system +
        idle +
        iowait +
        irq +
        softirq +
        steal;

    /*
     * First call only establishes the baseline.
     */
    if (previous_total == 0)
    {
        previous_total = total;
        previous_idle = idle_total;

        return 0.0f;
    }

    uint64_t total_delta =
        total - previous_total;

    uint64_t idle_delta =
        idle_total - previous_idle;

    previous_total = total;
    previous_idle = idle_total;

    if (total_delta == 0)
    {
        return 0.0f;
    }

    float usage =
        100.0f *
        (1.0f -
         ((float) idle_delta /
          (float) total_delta));

    if (usage < 0.0f)
    {
        usage = 0.0f;
    }

    if (usage > 100.0f)
    {
        usage = 100.0f;
    }

    return usage;
}
