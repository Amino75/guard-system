#ifndef SHARED_MEMORY_H
#define SHARED_MEMORY_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define SHM_NAME "/guard_system"

#define MAX_PATH_LENGTH 256
#define MAX_TIMESTAMP_LENGTH 64

/*
 * Camera frame configuration.
 *
 * 640 x 480 YUYV
 * 2 bytes per pixel
 *
 * 640 * 480 * 2 = 614400 bytes
 */
#define CAMERA_FRAME_WIDTH 640
#define CAMERA_FRAME_HEIGHT 480
#define CAMERA_FRAME_BYTES \
    (CAMERA_FRAME_WIDTH * CAMERA_FRAME_HEIGHT * 2)

/*
 * Maximum number of detected persons.
 */
#define MAX_DETECTIONS 10

/*
 * Bounding box produced by Python.
 *
 * Coordinates are relative to the
 * camera frame.
 */
typedef struct
{
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
    float confidence;

} detection_box_t;


/*
 * Data exchanged between processes.
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

    /*
     * Raw camera frame information.
     *
     * frame_sequence:
     *
     * Even value = stable frame
     * Odd value  = C is currently writing frame
     */
    uint32_t frame_sequence;

    uint32_t frame_width;

    uint32_t frame_height;

    uint32_t frame_size;

    /*
     * Latest raw YUYV camera frame.
     *
     * 640 * 480 * 2 = 614400 bytes
     */
    unsigned char frame_yuyv[CAMERA_FRAME_BYTES];

    /*
     * Detection data.
     *
     * detection_sequence:
     *
     * Even value = stable detection data
     * Odd value  = Python is currently writing
     */
    uint32_t detection_sequence;

    uint32_t detection_count;

    detection_box_t detections[MAX_DETECTIONS];

} telemetry_t;


/*
 * Initialize shared memory.
 */
int shm_init(void);


/*
 * Get shared memory pointer.
 */
telemetry_t *shm_get(void);


/*
 * Cleanup shared memory.
 */
void shm_destroy(void);

#endif
