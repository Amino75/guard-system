#ifndef CAMERA_H
#define CAMERA_H

#include <stddef.h>


int camera_init(void);


int camera_capture(void);


unsigned char *camera_get_jpeg(
    size_t *size
);


void camera_stop(void);


#endif
