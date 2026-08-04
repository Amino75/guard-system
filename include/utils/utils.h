#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


/*
 * Time utilities
 */

/* Get current timestamp string */
void utils_get_timestamp(char *buffer, size_t size);


/* Get monotonic time in milliseconds */
uint64_t utils_get_time_ms(void);


/*
 * Delay utilities
 */

/* Sleep for milliseconds */
void utils_sleep_ms(unsigned int ms);


/*
 * File utilities
 */

/* Check if file exists */
bool utils_file_exists(const char *path);


/* Create directory if it does not exist */
int utils_create_directory(const char *path);


/*
 * String utilities
 */

/* Safe string copy */
void utils_str_copy(char *dest,
                    const char *src,
                    size_t size);


/*
 * System utilities
 */

/* Read CPU temperature */
float utils_get_cpu_temperature(void);


#endif