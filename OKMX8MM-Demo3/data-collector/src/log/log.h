#ifndef __LOG_H

#define __LOG_H 1

#define M_LOG_DEBUG 0
#define M_LOG_INFO 1
#define M_LOG_WARN 2
#define M_LOG_ERROR 3

typedef struct m_log_config_struct {
    // log level filter
    int log_level;

    // rolling configuration
    char *log_file_name_prefix;
    int rolling_count;
    int rolling_file_size;

} m_log_config_t;

int m_log_init(m_log_config_t *m_log_config);
void m_log(int level, char *format, ...);

#endif