#ifndef HISTORY_H
#define HISTORY_H

#include <stdint.h>

#define HISTORY_MAX_ENTRIES 5

typedef struct
{
    char timestamp[64];
    uint32_t person_count;
    float cpu_temperature;
} history_entry_t;

int history_init(void);

int history_add(
    uint32_t person_count,
    float cpu_temperature
);

int history_get(
    history_entry_t *entries,
    uint32_t *count
);

void history_shutdown(void);

#endif
