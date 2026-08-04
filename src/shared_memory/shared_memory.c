#include "shared_memory/shared_memory.h"

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>



static int shm_fd = -1;


static telemetry_t *shared_data = NULL;



int shm_init(void)
{

    shm_fd = shm_open(
        SHM_NAME,
        O_CREAT | O_RDWR,
        0666
    );


    if(shm_fd < 0)
    {
        return -1;
    }



    if(ftruncate(
            shm_fd,
            sizeof(telemetry_t)) != 0)
    {
        close(shm_fd);
        return -1;
    }



    shared_data = mmap(
        NULL,
        sizeof(telemetry_t),
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        shm_fd,
        0
    );



    if(shared_data == MAP_FAILED)
    {
        shared_data = NULL;

        close(shm_fd);

        return -1;
    }



    memset(
        shared_data,
        0,
        sizeof(telemetry_t)
    );



    return 0;
}



telemetry_t *shm_get(void)
{
    return shared_data;
}




void shm_destroy(void)
{

    if(shared_data != NULL)
    {
        munmap(
            shared_data,
            sizeof(telemetry_t)
        );

        shared_data = NULL;
    }



    if(shm_fd >= 0)
    {

        close(shm_fd);


        shm_unlink(
            SHM_NAME
        );


        shm_fd = -1;
    }

}

