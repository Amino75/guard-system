#ifndef SHARED_MEMORY_H
#define SHARED_MEMORY_H

#include <stdbool.h>
#include <stdint.h>


#define SHM_NAME "/guard_system"


#define MAX_PATH_LENGTH 256
#define MAX_TIMESTAMP_LENGTH 64



/*
 * Data exchanged between processes
 */
typedef struct
{

    /*
     * Camera / detection information
     */

    uint32_t frame_number;

    uint32_t person_count;

    float fps;



    /*
     * System information
     */

    float cpu_temperature;



    /*
     * Guard status
     */

    bool guard_mode;

    bool person_detected;



    /*
     * Metadata
     */

    char timestamp[MAX_TIMESTAMP_LENGTH];

    char jpeg_path[MAX_PATH_LENGTH];


} telemetry_t;



/*
 * Initialize shared memory
 */
int shm_init(void);



/*
 * Get shared memory pointer
 */
telemetry_t *shm_get(void);



/*
 * Cleanup shared memory
 */
void shm_destroy(void);



#endif