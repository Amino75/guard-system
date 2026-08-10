#include "api/guard_api.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include "shared_memory/shared_memory.h"
#include "utils/utils.h"
#include <string.h>
#include <stdio.h>
#include <time.h>


int guard_api_get_telemetry(
    guard_telemetry_t *telemetry
)
{
    if (telemetry == NULL)
    {
        return -1;
    }


    float temperature =
        utils_get_cpu_temperature();


    uint64_t free_memory =
        utils_get_free_memory();


    float cpu_usage =
        utils_get_cpu_usage();


    if (temperature < 0.0f)
    {
        return -1;
    }


    if (free_memory == 0)
    {
        return -1;
    }


    if (cpu_usage < 0.0f)
    {
        return -1;
    }


    telemetry->cpu_temperature =
        temperature;


    /*
     * utils_get_free_memory()
     * returns bytes.
     *
     * API requirement:
     * return memory in MB.
     */
    telemetry->free_memory_mb =
        free_memory / (1024ULL * 1024ULL);


    telemetry->cpu_usage =
        cpu_usage;


    return 0;
}

/*
int guard_api_get_person_count(
    uint32_t *count
)
{
    if (count == NULL)
    {
        return -1;
    }


    telemetry_t *data =
        shm_get();


    if (data == NULL)
    {
        return -1;
    }


    *count =
        data->person_count;


    return 0;
}
*/

int guard_api_get_person_count(
    uint32_t *count
)
{
    if (count == NULL)
    {
        return -1;
    }

    int fd = shm_open(
        SHM_NAME,
        O_RDONLY,
        0
    );

    if (fd < 0)
    {
        return -1;
    }

    telemetry_t *data =
        mmap(
            NULL,
            sizeof(telemetry_t),
            PROT_READ,
            MAP_SHARED,
            fd,
            0
        );

    if (data == MAP_FAILED)
    {
        close(fd);
        return -1;
    }

    *count = data->person_count;

    munmap(
        data,
        sizeof(telemetry_t)
    );

    close(fd);

    return 0;
}

int guard_api_get_timestamp(
    char *buffer,
    uint32_t buffer_size
)
{
    if (
        buffer == NULL ||
        buffer_size == 0
    )
    {
        return -1;
    }


    time_t now =
        time(NULL);


    struct tm time_info;


    if (
        localtime_r(
            &now,
            &time_info
        ) == NULL
    )
    {
        return -1;
    }


    if (
        strftime(
            buffer,
            buffer_size,
            "%Y-%m-%dT%H:%M:%S",
            &time_info
        ) == 0
    )
    {
        return -1;
    }


    return 0;
}


int guard_api_get_stream_path(
    char *buffer,
    uint32_t buffer_size
)
{
    if (
        buffer == NULL ||
        buffer_size == 0
    )
    {
        return -1;
    }

    const char *path = "/stream";

    size_t length = strlen(path);

    if (length + 1 > buffer_size)
    {
        return -1;
    }

    memcpy(
        buffer,
        path,
        length + 1
    );

    return 0;
}

int guard_api_set_guard_mode(
    bool enabled
)
{
    int fd =
        shm_open(
            SHM_NAME,
            O_RDWR,
            0
        );

    if (fd < 0)
    {
        return -1;
    }

    telemetry_t *data =
        mmap(
            NULL,
            sizeof(telemetry_t),
            PROT_READ | PROT_WRITE,
            MAP_SHARED,
            fd,
            0
        );

    if (data == MAP_FAILED)
    {
        close(fd);
        return -1;
    }

    data->guard_mode = enabled;

    munmap(
        data,
        sizeof(telemetry_t)
    );

    close(fd);

    return 0;
}


int guard_api_get_guard_mode(
    bool *enabled
)
{
    if (enabled == NULL)
    {
        return -1;
    }

    int fd =
        shm_open(
            SHM_NAME,
            O_RDONLY,
            0
        );

    if (fd < 0)
    {
        return -1;
    }

    telemetry_t *data =
        mmap(
            NULL,
            sizeof(telemetry_t),
            PROT_READ,
            MAP_SHARED,
            fd,
            0
        );

    if (data == MAP_FAILED)
    {
        close(fd);
        return -1;
    }

    *enabled =
        data->guard_mode;

    munmap(
        data,
        sizeof(telemetry_t)
    );

    close(fd);

    return 0;
}

