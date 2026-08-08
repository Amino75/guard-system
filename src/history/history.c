#include "history/history.h"

#include "utils/utils.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#define HISTORY_FILE "/var/lib/guard_history.bin"

static history_entry_t history[HISTORY_MAX_ENTRIES];
static uint32_t history_count = 0;

static int history_save(void)
{
    FILE *fp = fopen(HISTORY_FILE, "wb");

    if (fp == NULL)
        return -1;

    size_t written = fwrite(
        history,
        sizeof(history_entry_t),
        HISTORY_MAX_ENTRIES,
        fp
    );

    fclose(fp);

    if (written != HISTORY_MAX_ENTRIES)
        return -1;

    return 0;
}

int history_init(void)
{
    FILE *fp = fopen(HISTORY_FILE, "rb");

    if (fp == NULL)
    {
        memset(
            history,
            0,
            sizeof(history)
        );

        history_count = 0;

        return 0;
    }

    size_t read_count = fread(
        history,
        sizeof(history_entry_t),
        HISTORY_MAX_ENTRIES,
        fp
    );

    fclose(fp);

    if (read_count > HISTORY_MAX_ENTRIES)
        read_count = HISTORY_MAX_ENTRIES;

    history_count = (uint32_t)read_count;

    return 0;
}

int history_add(
    uint32_t person_count,
    float cpu_temperature
)
{
    history_entry_t entry;

    memset(
        &entry,
        0,
        sizeof(entry)
    );

    utils_get_timestamp(
        entry.timestamp,
        sizeof(entry.timestamp)
    );

    entry.person_count = person_count;
    entry.cpu_temperature = cpu_temperature;

    if (history_count < HISTORY_MAX_ENTRIES)
    {
        history[history_count] = entry;
        history_count++;
    }
    else
    {
        memmove(
            &history[0],
            &history[1],
            sizeof(history_entry_t) *
            (HISTORY_MAX_ENTRIES - 1)
        );

        history[HISTORY_MAX_ENTRIES - 1] = entry;
    }

    return history_save();
}

int history_get(
    history_entry_t *entries,
    uint32_t *count
)
{
    if (entries == NULL || count == NULL)
        return -1;

    FILE *fp = fopen(HISTORY_FILE, "rb");

    if (fp == NULL)
    {
        *count = 0;
        return 0;
    }

    size_t read_count = fread(
        entries,
        sizeof(history_entry_t),
        HISTORY_MAX_ENTRIES,
        fp
    );

    fclose(fp);

    if (read_count > HISTORY_MAX_ENTRIES)
        read_count = HISTORY_MAX_ENTRIES;

    *count = (uint32_t)read_count;

    return 0;
}

void history_shutdown(void)
{
    memset(
        history,
        0,
        sizeof(history)
    );

    history_count = 0;
}
