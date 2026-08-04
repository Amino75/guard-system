#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>

int config_load(const char *filename);

void config_unload(void);

const char *config_get_string(const char *path);

int config_get_int(const char *path);

double config_get_double(const char *path);

bool config_get_bool(const char *path);

#endif