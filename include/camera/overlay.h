#ifndef OVERLAY_H
#define OVERLAY_H

#include <stddef.h>
#include <stdint.h>

#include "shared_memory/shared_memory.h"

void overlay_draw_detections(
    unsigned char *yuyv,
    int width,
    int height,
    const detection_box_t *detections,
    uint32_t count,
    const char *student_id,
    double stream_fps
);

#endif
