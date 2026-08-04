#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>

typedef enum
{
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR
} log_level_t;


/*
 * Initialize logger
 *
 * filename:
 * path to log file
 *
 * level:
 * minimum displayed level
 */
int log_init(const char *filename, log_level_t level);


/*
 * Close logger and release resources
 */
void log_close(void);


/*
 * Logging functions
 */
void log_debug(const char *format, ...);

void log_info(const char *format, ...);

void log_warning(const char *format, ...);

void log_error(const char *format, ...);


#endif