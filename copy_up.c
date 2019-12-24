#include "copy_up.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

int copy_up(const char *source_path, const char *upper_path) {
    char *upper_dir = file_path_dir(upper_path);
    if (access(upper_dir, F_OK)) {
        // upper dir does not exist
        int res;
        if (res = mkdirs(upper_dir))
            return res;
    }
    FILE *f_read, *f_write;
    const int buffer_len = 1024;
    char *buffer = (char *)malloc(sizeof(char) * buffer_len);
    int read_cnt;
    if (((f_read = fopen(source_path, "rb")) == NULL) || ((f_write = fopen(upper_path, "wb")) == NULL))
        return -1;

    while ((read_cnt = fread(buffer, sizeof(char), buffer_len, f_read)) > 0)
        fwrite(buffer, read_cnt, sizeof(char), f_write);

    free(buffer);
    fclose(f_read);
    fclose(f_write);
    free(upper_dir);
    return 0;
}