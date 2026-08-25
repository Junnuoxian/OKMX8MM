#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/stat.h>

#include "signal-handler/signal-handler.h"
#include "util/util.h"
#include "log.h"

// type
typedef struct m_log_context_struct {
    // log level filter
    int log_level;

    // rolling configuration
    char *log_file_name_prefix;
    int rolling_count;
    int rolling_file_size;

    // current log file descriptor
    int current_log_fd;

    // log ticks
    int log_ticks;

    // shutdown flag
    int shutdown_flag;

} m_log_context_t;

// static
static m_log_context_t m_log_context;
static pthread_mutex_t mutex;

// function delaration
int do_rolling();
void do_m_log(int level, char *format, va_list args);

// implementation
void on_shutdown() {
    // set the shutdown flag
    m_log_context.shutdown_flag = 1;

    // close the file
    if (m_log_context.current_log_fd != 0) {
        close(m_log_context.current_log_fd);
    }
    m_log_context.current_log_fd = 0;
}

int m_log_init(m_log_config_t *m_log_config) {
    m_log_context.log_level = m_log_config->log_level;
    m_log_context.log_file_name_prefix = m_log_config->log_file_name_prefix;
    m_log_context.rolling_count = m_log_config->rolling_count;
    m_log_context.rolling_file_size = m_log_config->rolling_file_size;

    m_log_context.current_log_fd = 0;
    m_log_context.log_ticks = 0;
    m_log_context.shutdown_flag = 0;

    // init mutex
    int mutex_init_ret = pthread_mutex_init(&mutex, NULL);
    if (mutex_init_ret != 0) {
        m_log(M_LOG_ERROR, "Failed to initialize mutex for logging.");
        return -1;
    }

    // add shutdown hook
    add_shutdown_callback(on_shutdown);

    // start rolling
    return rolling();
}

int rolling() {
    /*
    int lock_ret = pthread_mutex_lock(&mutex);
    if (lock_ret != 0) {
        printf("Failed to lock mutex of log rolling");
        return -1;
    }
    */
    int do_rolling_ret = do_rolling();
    /*
    int unlock_ret = pthread_mutex_unlock(&mutex);
    if (unlock_ret != 0) {
        printf("Failed to unlock mutex of log rolling");
        return -1;
    }
    */
    return do_rolling_ret;
}

int do_rolling() {
    char log_file_name_buffer[256];
    memset(log_file_name_buffer, 0, sizeof(log_file_name_buffer));
    
    sprintf(log_file_name_buffer, "%s-%d.log", m_log_context.log_file_name_prefix, 0);
    int log_file_0_fd = open(log_file_name_buffer, O_RDWR | O_CREAT | O_APPEND);
    if (log_file_0_fd < 0) {
        printf("Failed to open log file '%s'\n", log_file_name_buffer);
        return -1;
    }
    
    struct stat log_file_0_stat;
    int fstat_ret = fstat(log_file_0_fd, &log_file_0_stat);
    close(log_file_0_fd);
    if (fstat_ret < 0) {
        printf("Failed to get stat of log file '%s'\n", log_file_name_buffer);
        return -1;
    }

    int log_file_size = log_file_0_stat.st_size;
    if (log_file_size > m_log_context.rolling_file_size) {
        printf("Starting rolling log files\n");
        close(m_log_context.current_log_fd);
        
        for(int i=m_log_context.rolling_count-1;i>=0;i--) {
            sprintf(log_file_name_buffer, "%s-%d.log", m_log_context.log_file_name_prefix, i);
            int access_ret = access(log_file_name_buffer, F_OK);
            if (access_ret == 0) {
                if (i == m_log_context.rolling_count-1) {
                    // delete the oldest file
                    int remove_ret = remove(log_file_name_buffer);
                    if (remove_ret != 0) {
                        printf("Failed to remove file '%s', will ignore and continue.", log_file_name_buffer);
                    }
                } else {
                    // rename file
                    char new_file_name_buffer[512];
                    memset(new_file_name_buffer, 0, sizeof(new_file_name_buffer));
                    sprintf(new_file_name_buffer, "%s-%d.log", m_log_context.log_file_name_prefix, i+1);
                    int rename_ret = rename(log_file_name_buffer, new_file_name_buffer);
                    if (rename_ret != 0) {
                        printf("Failed to rename file '%s' to '%s', will ignore and continue.", log_file_name_buffer, new_file_name_buffer);
                    }
                }
            }
        }
    } else if (m_log_context.current_log_fd != 0) {
        return 0;
    }
    
    m_log_context.current_log_fd = 0;
    sprintf(log_file_name_buffer, "%s-%d.log", m_log_context.log_file_name_prefix, 0);
    int current_log_fd = open(log_file_name_buffer, O_RDWR | O_CREAT | O_APPEND);
    if (current_log_fd < 0) {
        printf("Failed to open log file '%s'\n", log_file_name_buffer);
        return -1;
    }
    m_log_context.current_log_fd = current_log_fd;
    return 0;
}

void m_log(int level, char *format, ...) {
    int lock_ret = pthread_mutex_lock(&mutex);
    if (lock_ret != 0) {
        m_log(M_LOG_ERROR, "Failed to lock mutex of log rolling");
        return;
    }
    
    va_list args;
    va_start(args, format);
    do_m_log(level, format, args);
    va_end(args);

    int unlock_ret = pthread_mutex_unlock(&mutex);
    if (unlock_ret != 0) {
        m_log(M_LOG_ERROR, "Failed to unlock mutex of log rolling");
        return;
    }
}

void do_m_log(int level, char *format, va_list args) {
    if (level < m_log_context.log_level) {
        return;
    }

    // rolling check
    if (m_log_context.log_ticks++ > 100) {
        m_log_context.log_ticks = 0;
        rolling();
    }

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

    char *log_level_str = "INFO";
    switch (level) {
        case M_LOG_DEBUG:
            log_level_str = "DEBUG";
            break;
        case M_LOG_INFO:
            log_level_str = "INFO";
            break;
        case M_LOG_WARN:
            log_level_str = "WARN";
            break;
        case M_LOG_ERROR:
            log_level_str = "ERROR";
            break;
    }

    char time_str_buffer[64];
    memset(time_str_buffer, 0, sizeof(time_str_buffer));

    char pthread_name[512];
    int get_pthread_name_ret = pthread_getname_np(pthread_self(), pthread_name, sizeof(pthread_name));
    if (get_pthread_name_ret != 0) {
        sprintf(pthread_name, "MAIN");
    }
    for (int i=0, len = strlen(pthread_name); i<len; i++) {
        pthread_name[i] = toupper(pthread_name[i]);
    }

    sprintf(time_str_buffer, "[%04d-%02d-%02d %02d:%02d:%02d.%03d][%-5s][%-15s] ", year, month, date, hour, min, sec, milliseconds, log_level_str, pthread_name);;

    char log_format[1024];
    memset(log_format, 0, sizeof(log_format));
    strcat(log_format, time_str_buffer);
    strcat(log_format, format);

    if (log_format[strlen(log_format) - 1] != '\n') {
        strcat(log_format, "\n");
    }

    // log buffer
    char log_buffer[2048];
    memset(log_buffer, 0, sizeof(log_buffer));

    // va_list args;
    // va_start(args, format);
    vsnprintf(log_buffer, sizeof(log_buffer), log_format, args);
    // va_end(args);

    printf(log_buffer);
    fflush(stdout);
    if (m_log_context.shutdown_flag == 0) {
        write(m_log_context.current_log_fd, log_buffer, strlen(log_buffer));
    }
}
