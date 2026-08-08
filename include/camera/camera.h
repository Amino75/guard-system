#ifndef CAMERA_H
#define CAMERA_H

#include <stddef.h>
#include <stdint.h>

int camera_init(void);


int camera_capture(void);


unsigned char *camera_get_jpeg(
    size_t *size
);
uint64_t camera_get_frame_id(void);

void camera_stop(void);


#endif
