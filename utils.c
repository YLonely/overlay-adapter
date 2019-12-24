#define _GNU_SOURCE
#include "utils.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

char *target_path(const char *path, const char *prefix, const size_t prefix_len) {
    const size_t size = strlen(path);
    const size_t dst_len = prefix_len + size + 1;
    char *dst = (char *)malloc(sizeof(char) * dst_len);

    memcpy(dst, prefix, prefix_len);
    memcpy(dst + prefix_len, path, size + 1);
    return dst;
}

char *source_file_path(const char *b) {
    return target_path(b, adapter_config.source_dir, adapter_config.source_dir_len);
}

char *upper_file_path(const char *b) { return target_path(b, adapter_config.upper_dir, adapter_config.upper_dir_len); }

char *file_path_dir(const char *path) {
    const size_t size = strlen(path);
    char *p = strrchr(path, '/');
    const size_t dir_len = size - strlen(p) + 1;
    char *dst = (char *)malloc(sizeof(char) * dir_len);

    memcpy(dst, path, dir_len);
    dst[dir_len - 1] = 0;
    return dst;
}

int exists(const char *path) {
    int res;
    if (!access(path, F_OK))
        res = 1;
    else
        res = 0;
    return res;
}

const char *abs_dir_path(const char *path) {
    char *abs_path;
    if (strlen(path) == 0)
        return NULL;
    if (path[0] == '/') {
        abs_path = strdup(path);
        return abs_path;
    }
    char *pwd = get_current_dir_name();
    size_t len = strlen(pwd) + 1 + strlen(path) + 1;
    int has_trailing = (path[strlen(path) - 1] == '/');
    if (!has_trailing)
        len++;
    abs_path = (char *)malloc(sizeof(char) * len);
    memset(abs_path, 0, len);
    sprintf(abs_path, "%s/%s", pwd, path);

    if (!has_trailing)
        abs_path[len - 2] = '/';
    return abs_path;
}

int mkdirs(char *path) {
    if (path[0] != '/')
        return -1;
    const size_t len = strlen(path);
    int i;
    for (i = 1; i < len; i++) {
        if (path[i] == '/') {
            path[i] = 0;
            if (access(path, F_OK))
                mkdir(path, 0644);
            path[i] = '/';
        }
    }
    if (access(path, F_OK))
        mkdir(path, 0644);
    return 0;
}