
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/types.h>

#include "log/log.h"
#include "led/led.h"
#include "file-storage.h"

int64_t file_storage_get_free_space(file_storage_context_t *file_storage_context) {
    struct statfs stat;
    int statfs_ret = statfs(file_storage_context->file_storage_config->path, &stat);
    if (statfs_ret < 0) {
        m_log(M_LOG_ERROR, "Failed to retrieve free space information of %s, ret=%d", file_storage_context->file_storage_config->path, statfs_ret);
        return -1;
    }

    int64_t bytes_free = (int64_t) stat.f_bfree * (int64_t) stat.f_bsize;
    return bytes_free;
}

int file_storage_rolling(file_storage_context_t *file_storage_context) {
    if (strstr(file_storage_context->file_storage_config->path, "/media/sdcard/") != NULL) {
        int sdcard_mounted = file_storage_check_sdcard_mount();
        if (sdcard_mounted != 0) {
            m_log(M_LOG_ERROR, "SD card not mounted, stop rolling for '%s'", file_storage_context->file_storage_config->path);
            return 0;
        }
    }

    int64_t bytes_free = file_storage_get_free_space(file_storage_context);
    if (bytes_free < 0) {
        return -1;
    }
    if (bytes_free > file_storage_context->file_storage_config->rolling_free_space) {
        // nop
        return 0;
    }

    // log
    m_log(M_LOG_INFO, "Free space %d.%dGB < %dGB, start rolling files...",
        (int) (bytes_free >> 30), (int) (bytes_free >> 20) % 1024,
        file_storage_context->file_storage_config->rolling_free_space >> 30);

    time_t time_now_seconds = time(NULL);
    struct tm *time_now = localtime(&time_now_seconds);
    time_now->tm_hour = 0;
    time_now->tm_min = 0;
    time_now->tm_sec = 0;
    int64_t max_timestamp = mktime(time_now);
    max_timestamp *= 1000LL;

    // loop
    int has_error = 0;
    int64_t last_bytes_free = 0;
    while(1) {
        bytes_free = file_storage_get_free_space(file_storage_context);
        if (bytes_free < file_storage_context->file_storage_config->rolling_free_space) {
            if (bytes_free == last_bytes_free) {
                break;
            }
            last_bytes_free = bytes_free;

            // delete old dated files if free space lower than then water level
            // find the oldest folder by name
            DIR *root_dir = opendir(file_storage_context->file_storage_config->path);
            if (root_dir==NULL) {
                m_log(M_LOG_ERROR, "Failed to open directory '%s'", file_storage_context->file_storage_config->path);
                return -1;
            }

            // oldest month folder
            int64_t oldest_month_folder_timestamp = INT64_MAX;
            struct tm oldest_month_folder_tm;
            char oldest_month_folder_path[FILE_PATH_LENGTH] = { 0 };

            struct dirent *month_dir_entry;
            while ((month_dir_entry = readdir(root_dir)) != NULL) {
                if (strcmp(month_dir_entry->d_name, ".") == 0 || strcmp(month_dir_entry->d_name, "..") == 0 || month_dir_entry->d_type != DT_DIR) {
                    continue;
                }

                struct tm month_folder_tm; memset(&month_folder_tm, 0, sizeof(struct tm));
                int input_elements = sscanf(month_dir_entry->d_name, "%d-%02d", &(month_folder_tm.tm_year), &(month_folder_tm.tm_mon));
                if (input_elements < 2) {
                    m_log(M_LOG_WARN, "Invalid folder name '%s'", month_dir_entry->d_name);
                    continue;
                }
                month_folder_tm.tm_year -= 1900;
                month_folder_tm.tm_mon -= 1;
                month_folder_tm.tm_mday = 1;
                month_folder_tm.tm_hour = 0;
                month_folder_tm.tm_min = 0;
                month_folder_tm.tm_sec = 0;

                int64_t month_folder_timestamp = mktime(&month_folder_tm);
                month_folder_timestamp *= 1000LL;
                if (month_folder_timestamp < oldest_month_folder_timestamp) {
                    oldest_month_folder_timestamp = month_folder_timestamp;
                    oldest_month_folder_tm = month_folder_tm;
                    sprintf(oldest_month_folder_path, "%s/%s", file_storage_context->file_storage_config->path, month_dir_entry->d_name);
                }
            }

            // close root folder
            closedir(root_dir);
            m_log(M_LOG_INFO, "Oldest month folder: %s", oldest_month_folder_path);

            // find oldest date folder within month folder
            int64_t oldest_folder_timestamp = INT64_MAX;
            char oldest_folder_path[FILE_PATH_LENGTH] = { 0 };
            if (oldest_month_folder_timestamp < INT64_MAX) {
                DIR *month_dir = opendir(oldest_month_folder_path);
                if (month_dir == NULL) {
                    m_log(M_LOG_ERROR, "Failed to open directory '%s'", oldest_month_folder_path);
                    has_error = 1;
                } else {
                    struct dirent *date_dir_entry;
                    while ((date_dir_entry = readdir(month_dir)) != NULL) {
                        if (strcmp(date_dir_entry->d_name, ".") == 0 || strcmp(date_dir_entry->d_name, "..") == 0 || date_dir_entry->d_type != DT_DIR) {
                            continue;
                        }

                        struct tm date_folder_tm = oldest_month_folder_tm;
                        int input_elements = sscanf(date_dir_entry->d_name, "%02d", &(date_folder_tm.tm_mday));
                        if (input_elements < 1) {
                            m_log(M_LOG_WARN, "Invalid folder name '%s'", date_dir_entry->d_name);
                            continue;
                        }

                        int64_t date_folder_timestamp = mktime(&date_folder_tm);
                        date_folder_timestamp *= 1000LL;
                        if (date_folder_timestamp < oldest_folder_timestamp) {
                            oldest_folder_timestamp = date_folder_timestamp;
                            sprintf(oldest_folder_path, "%s/%s", oldest_month_folder_path, date_dir_entry->d_name);
                        }
                    }
                    closedir(month_dir);
                }
            }
            m_log(M_LOG_INFO, "Oldest date folder: %s", oldest_folder_path);

            if (oldest_folder_timestamp >= max_timestamp) {
                m_log(M_LOG_WARN, "Oldest folder timestamp larger than today, ignored.");
                break;
            }

            // remove oldest folder
            if (oldest_folder_timestamp != INT64_MAX) {
                file_storage_remove_directory_recursive_force(oldest_folder_path);
            }

            if (has_error) {
                break;
            }
        } else {
            break;
        }
    }

    // return
    return has_error ? -1 : 0;
}

int file_storage_compress(file_storage_context_t *file_storage_context) {
    if (!file_storage_context->file_storage_config->compress) {
        return 0;
    }
    
    time_t time_now_seconds = time(NULL);
    struct tm *time_now = localtime(&time_now_seconds);
    time_now->tm_hour = 0;
    time_now->tm_min = 0;
    time_now->tm_sec = 0;
    int64_t max_timestamp = mktime(time_now);
    max_timestamp *= 1000LL;

    // loop
    int has_error = 0;
    while(1) {
        // read month folders
        DIR *root_dir = opendir(file_storage_context->file_storage_config->path);
        if (root_dir==NULL) {
            m_log(M_LOG_ERROR, "Failed to open directory '%s'", file_storage_context->file_storage_config->path);
            return -1;
        }

        // oldest month folder
        int64_t closest_month_folder_timestamp = 0;
        struct tm closest_month_folder_tm;
        char closest_month_folder_path[FILE_PATH_LENGTH] = { 0 };

        struct dirent *month_dir_entry;
        while ((month_dir_entry = readdir(root_dir)) != NULL) {
            if (strcmp(month_dir_entry->d_name, ".") == 0 || strcmp(month_dir_entry->d_name, "..") == 0 || month_dir_entry->d_type != DT_DIR) {
                continue;
            }

            struct tm month_folder_tm; memset(&month_folder_tm, 0, sizeof(struct tm));
            int input_elements = sscanf(month_dir_entry->d_name, "%d-%02d", &(month_folder_tm.tm_year), &(month_folder_tm.tm_mon));
            if (input_elements < 2) {
                m_log(M_LOG_WARN, "Invalid folder name '%s'", month_dir_entry->d_name);
                continue;
            }
            month_folder_tm.tm_year -= 1900;
            month_folder_tm.tm_mon -= 1;
            month_folder_tm.tm_mday = 1;
            month_folder_tm.tm_hour = 0;
            month_folder_tm.tm_min = 0;
            month_folder_tm.tm_sec = 0;

            int64_t month_folder_timestamp = mktime(&month_folder_tm);
            month_folder_timestamp *= 1000LL;
            if (month_folder_timestamp >= max_timestamp) {
                continue;
            }
            if (month_folder_timestamp > closest_month_folder_timestamp) {
                closest_month_folder_timestamp = month_folder_timestamp;
                closest_month_folder_tm = month_folder_tm;
                sprintf(closest_month_folder_path, "%s/%s", file_storage_context->file_storage_config->path, month_dir_entry->d_name);
            }
        }

        // close root folder
        closedir(root_dir);
        m_log(M_LOG_INFO, "Closest month folder: %s", closest_month_folder_path);

        // find closest date folder within month folder
        int64_t closest_folder_timestamp = 0;
        char closest_folder_path[FILE_PATH_LENGTH] = { 0 };
        if (closest_month_folder_timestamp > 0) {
            DIR *month_dir = opendir(closest_month_folder_path);
            if (month_dir == NULL) {
                m_log(M_LOG_ERROR, "Failed to open directory '%s'", closest_month_folder_path);
                has_error = 1;
            } else {
                struct dirent *date_dir_entry;
                while ((date_dir_entry = readdir(month_dir)) != NULL) {
                    if (strcmp(date_dir_entry->d_name, ".") == 0 || strcmp(date_dir_entry->d_name, "..") == 0 || date_dir_entry->d_type != DT_DIR) {
                        continue;
                    }

                    struct tm date_folder_tm = closest_month_folder_tm;
                    int input_elements = sscanf(date_dir_entry->d_name, "%02d", &(date_folder_tm.tm_mday));
                    if (input_elements < 1) {
                        m_log(M_LOG_WARN, "Invalid folder name '%s'", date_dir_entry->d_name);
                        continue;
                    }

                    int64_t date_folder_timestamp = mktime(&date_folder_tm);
                    date_folder_timestamp *= 1000LL;
                    if (date_folder_timestamp >= max_timestamp) {
                        continue;
                    }
                    if (date_folder_timestamp > closest_folder_timestamp) {
                        closest_folder_timestamp = date_folder_timestamp;
                        sprintf(closest_folder_path, "%s/%s", closest_month_folder_path, date_dir_entry->d_name);
                    }
                }
                closedir(month_dir);
            }
        }
        m_log(M_LOG_INFO, "Closest date folder: %s", closest_folder_path);

        // compress the files within folder
        int folder_has_zip = 0;
        if (closest_folder_timestamp > 0) {
            while(1) {
                DIR *closest_dir = opendir(closest_folder_path);
                if (closest_dir == NULL) {
                    m_log(M_LOG_ERROR, "Failed to open directory '%s'", closest_month_folder_path);
                    has_error = 1;
                    break;
                } else {
                    // find the file to compress
                    int has_zip = 0;
                    struct dirent *file_entry;
                    char file_path[FILE_PATH_LENGTH] = { 0 };
                    while ((file_entry = readdir(closest_dir)) != NULL) {
                        if (strcmp(file_entry->d_name, ".") == 0 || strcmp(file_entry->d_name, "..") == 0 || file_entry->d_type != DT_REG) {
                            continue;
                        }
                        char *suffix = ".bin";
                        int suffix_len = strlen(suffix);
                        int file_name_len = strlen(file_entry->d_name);
                        if (file_name_len < suffix_len || strcmp(file_entry->d_name + file_name_len - suffix_len, suffix) != 0) {
                            continue;
                        }

                        // mark the file
                        sprintf(file_path, "%s/%s", closest_folder_path, file_entry->d_name);
                        has_zip = 1;
                        folder_has_zip = 1;
                        break;
                    }
                    closedir(closest_dir);

                    // do compression
                    if (strlen(file_path) > 0) {
                        char command[512];
                        memset(command, 0, sizeof(command));
                        sprintf(command, "tar -cvzf %s.tar.gz %s", file_path, file_path);
                        m_log(M_LOG_INFO, "Start compression with command: %s", command);

                        int zip_ret = system(command);
                        if (zip_ret != 0) {
                            m_log(M_LOG_ERROR, "Failed to compress file, command: %s, ret=%d", command, zip_ret);
                            has_error = 1;
                            break;
                        }

                        // delete the original file
                        sprintf(command, "rm -rf %s", file_path);
                        int rm_ret = system(command);
                        if (rm_ret != 0) {
                            m_log(M_LOG_ERROR, "Failed to delete file, command: %s, ret=%d", command, rm_ret);
                            has_error = 1;
                            break;
                        }
                    }

                    // not files to zip
                    if (!has_zip) {
                        break;
                    }
                }
            }
        }

        if (has_error) {
            break;
        }

        // next loop from this timestamp
        max_timestamp = closest_folder_timestamp;
        if (!folder_has_zip) {
            break;
        }
    }

    // return
    return has_error ? -1 : 0;
}

int do_file_storage_write(file_storage_context_t *file_storage_context, char *buf, int buf_len) {
    if (file_storage_context->disabled) {
        return 0;
    }
    if (file_storage_context->file_fd >= 0) {
        time_t time_now_seconds = time(NULL);
        struct tm *time_now = localtime(&time_now_seconds);
        if (time_now->tm_mday != file_storage_context->file_tm.tm_mday
        || time_now->tm_mon != file_storage_context->file_tm.tm_mon
        || time_now->tm_year != file_storage_context->file_tm.tm_year) {
            // date change
            int close_ret = file_storage_close(file_storage_context);
            if (close_ret != 0) {
                m_log(M_LOG_ERROR, "Failed to close file storage context '%s'", file_storage_context->file_path);
            }
        }
    }

    if (file_storage_context->file_fd < 0) {
        // open file
        time_t time_now_seconds = time(NULL);
        struct tm *time_now = localtime(&time_now_seconds);

        int64_t time_now_millis = get_time_millis();
        int milliseconds = time_now_millis % 1000;

        int year = time_now->tm_year + 1900;
        int month = time_now->tm_mon + 1;
        int date = time_now->tm_mday;
        int hour = time_now->tm_hour;
        int min = time_now->tm_min;
        int sec = time_now->tm_sec;
        
        char dir_path[FILE_PATH_LENGTH];
        sprintf(dir_path, "%s/%04d-%02d/%02d", file_storage_context->file_storage_config->path, year, month, date);
        int mkdirs_ret = mkdirs(dir_path);
        if (mkdirs_ret < 0) {
            m_log(M_LOG_ERROR, "Failed to create directory %s, ret=%d", dir_path, mkdirs_ret);
            return -1;
        }

        char file_path[FILE_PATH_LENGTH];
        sprintf(file_path, "%s/%s-%04d%02d%02d%02d%02d%02d.bin", dir_path, file_storage_context->file_storage_config->file_name, year, month, date, hour, min, sec);
        int fd = open(file_path, O_RDWR | O_CREAT | O_APPEND);
        if (fd < 0) {
            m_log(M_LOG_ERROR, "Failed to open data file '%s'\n", file_path);
            return -1;
        }

        file_storage_context->file_fd = fd;
        file_storage_context->file_tm = *time_now;
        strcpy(file_storage_context->file_path, file_path);
    }

    // do write
    int written = 0;
    while(written < buf_len) {
        int this_written = write(file_storage_context->file_fd, buf + written, buf_len - written);
        if (this_written < 0) {
            m_log(M_LOG_ERROR, "Failed to write data to file %s, ret=%d", file_storage_context->file_path, this_written);
            return -1;
        }
        written += this_written;
    }

    return 0;
}

int file_storage_write(file_storage_context_t *file_storage_context, char *buf, int buf_len) {
    int write_ret = do_file_storage_write(file_storage_context, buf, buf_len);
    led_set_storage_error_status(write_ret == 0 ? STATUS_NORMAL : STATUS_ERROR);
    return write_ret;
}

int file_storage_close(file_storage_context_t *file_storage_context) {
    if (file_storage_context->file_fd > -1) {
        int close_ret = close(file_storage_context->file_fd);
        if (close_ret < 0) {
            m_log(M_LOG_ERROR, "Failed to close data file %s, ret=%d", file_storage_context->file_path, close_ret);
        }

        file_storage_context->file_fd = -1;
        file_storage_context->file_path[0] = 0;
        memset(&file_storage_context->file_tm, 0, sizeof(struct tm));
    }
    return 0;
}

int file_storage_check_sdcard_mount() {
    FILE *fp = fopen("/proc/mounts", "r");
    if (fp == NULL) {
        m_log(M_LOG_ERROR, "Failed to open /proc/mounts");
        led_set_storage_error_status(STATUS_ERROR);
        return -1;
    }

    int found = 0;
    char line[1024];
    while (fgets(line, sizeof(line)-1, fp) != NULL) {
        if (strstr(line, "/media/sdcard") != NULL) {
            found = 1;
            break;
        }
    }
    fclose(fp);

    if (!found) {
        m_log(M_LOG_ERROR, "SD card mount point 'media/sdcard' not found.");
    }
    
    // set led status
    led_set_storage_error_status(found ? STATUS_NORMAL : STATUS_ERROR);
    return found ? 0 : 1;
}

int file_storage_remove_directory_recursive_force(const char *path) {
    if (path == NULL) {
        return 0;
    }
    
    // remove with executing command
    char command[512];
    memset(command, 0, sizeof(command));
    sprintf(command, "rm -rf %s", path);
    m_log(M_LOG_INFO, "Removing directory with command: %s", command);

    int rm_ret = system(command);
    if (rm_ret != 0) {
        m_log(M_LOG_ERROR, "Failed to remove directory, command: %s, ret=%d", command, rm_ret);
        return -1;
    }
    return 0;
}

int mkdirs(const char *path) {
    char temp_path[FILE_PATH_LENGTH];
    memset(temp_path, 0, sizeof(temp_path));

    // check and create directories
    for(int i=0, len=strlen(path); i<len; i++) {
        if((path[i]=='/' && i>0) || i==len-1) {
            // check directory exists and create it if not
            memset(temp_path, 0, sizeof(temp_path));
            if (i==len-1 && path[i]!='/') {
                strcpy(temp_path, path);
            } else {
                strncpy(temp_path, path, i);
            }

            if (access(temp_path, F_OK) != 0) {
                int mkdir_ret = mkdir(temp_path, 0x777);
                if (mkdir_ret < 0) {
                    m_log(M_LOG_ERROR, "Failed to create directory '%s'", temp_path);
                    return -1;
                }
            }
        }
    }
    return 0;
}


