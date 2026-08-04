#include "common/config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cJSON.h"

static cJSON *config_root = NULL;

/*
 * Find a JSON node using a dot-separated path.
 * Example:
 * "logging.file"
 * "mqtt.host"
 * "application.version"
 */
static cJSON *config_find_node(const char *path)
{
    if (config_root == NULL || path == NULL)
    {
        return NULL;
    }

    char buffer[256];

    strncpy(buffer, path, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';

    cJSON *node = config_root;

    char *token = strtok(buffer, ".");

    while (token != NULL)
    {
        node = cJSON_GetObjectItemCaseSensitive(node, token);

        if (node == NULL)
        {
            return NULL;
        }

        token = strtok(NULL, ".");
    }

    return node;
}

int config_load(const char *filename)
{
    FILE *fp = fopen(filename, "rb");

    if (fp == NULL)
    {
        return -1;
    }

    fseek(fp, 0, SEEK_END);

    long file_size = ftell(fp);

    rewind(fp);

    if (file_size <= 0)
    {
        fclose(fp);
        return -1;
    }

    char *content = malloc((size_t)file_size + 1);

    if (content == NULL)
    {
        fclose(fp);
        return -1;
    }

    size_t bytes_read = fread(content, 1, (size_t)file_size, fp);

    fclose(fp);

    if (bytes_read != (size_t)file_size)
    {
        free(content);
        return -1;
    }

    content[file_size] = '\0';

    cJSON *root = cJSON_Parse(content);

    free(content);

    if (root == NULL)
    {
        return -1;
    }

    if (config_root != NULL)
    {
        cJSON_Delete(config_root);
    }

    config_root = root;

    return 0;
}

void config_unload(void)
{
    if (config_root != NULL)
    {
        cJSON_Delete(config_root);
        config_root = NULL;
    }
}

const char *config_get_string(const char *path)
{
    cJSON *node = config_find_node(path);

    if (node == NULL)
    {
        return NULL;
    }

    if (!cJSON_IsString(node))
    {
        return NULL;
    }

    return node->valuestring;
}

int config_get_int(const char *path)
{
    cJSON *node = config_find_node(path);

    if (node == NULL)
    {
        return 0;
    }

    if (!cJSON_IsNumber(node))
    {
        return 0;
    }

    return node->valueint;
}

double config_get_double(const char *path)
{
    cJSON *node = config_find_node(path);

    if (node == NULL)
    {
        return 0.0;
    }

    if (!cJSON_IsNumber(node))
    {
        return 0.0;
    }

    return node->valuedouble;
}

bool config_get_bool(const char *path)
{
    cJSON *node = config_find_node(path);

    if (node == NULL)
    {
        return false;
    }

    if (!cJSON_IsBool(node))
    {
        return false;
    }

    return cJSON_IsTrue(node);
}