#ifndef GUARD_API_H
#define GUARD_API_H

#include <stdint.h>

/*
 * Telemetry returned by the C API.
 */
typedef struct
{
    float cpu_temperature;
    uint64_t free_memory_mb;
    float cpu_usage;
} guard_telemetry_t;


/*
 * Get current system telemetry.
 *
 * Returns:
 *   0  success
 *  -1  failure
 */
int guard_api_get_telemetry(
    guard_telemetry_t *telemetry
);


/*
 * Get current detected person count.
 *
 * Returns:
 *   0  success
 *  -1  failure
 */
int guard_api_get_person_count(
    uint32_t *count
);


/*
 * Get ISO-style current timestamp.
 *
 * Example:
 *   2026-08-08T12:30:45
 *
 * Returns:
 *   0  success
 *  -1  failure
 */
int guard_api_get_timestamp(
    char *buffer,
    uint32_t buffer_size
);


/*
 * Get the C MJPEG stream endpoint.
 *
 * Returns:
 * 0  success
 * -1 failure
 */
int guard_api_get_stream_path(
    char *buffer,
    uint32_t buffer_size
);
#endif
