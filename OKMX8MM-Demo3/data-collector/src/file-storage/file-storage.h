#ifndef __FILE_STORAGE_H

#define __FILE_STORAGE_H

#include <stdint.h>
#include <time.h>

#define FILE_PATH_LENGTH 128

typedef struct file_storage_config_struct {
    char *path;
    char *file_name;

    int compress;
    int64_t rolling_free_space;

} file_storage_config_t;

typedef struct file_storage_context_struct {
    // config
    file_storage_config_t *file_storage_config;

    // runtime
    char file_path[FILE_PATH_LENGTH];
    int file_fd;
    struct tm file_tm;
    
    // disabled, for sdcard not mounted
    int disabled;

} file_storage_context_t;

int file_storage_rolling(file_storage_context_t *file_storage_context);
int file_storage_compress(file_storage_context_t *file_storage_context);
int file_storage_write(file_storage_context_t *file_storage_context, char *buf, int buf_len);
int file_storage_close(file_storage_context_t *file_storage_context);
int file_storage_remove_directory_recursive_force(const char *path);
int file_storage_check_sdcard_mount();
int mkdirs(const char *path);

#endif
