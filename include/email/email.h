#ifndef EMAIL_H
#define EMAIL_H

#include <stddef.h>

int email_init(void);

int email_send_alert(
    unsigned int person_count,
    const char *timestamp,
    double cpu_temperature,
    const unsigned char *jpeg_data,
    size_t jpeg_size
);

void email_cleanup(void);

#endif
