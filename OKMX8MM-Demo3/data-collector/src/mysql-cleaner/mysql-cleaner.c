#include <stdio.h>
#include <string.h>
#include <mysql.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/types.h>

#include "log/log.h"
#include "util/util.h"
#include "signal-handler/signal-handler.h"
#include "mysql-cleaner.h"

// type declaration
typedef struct mysql_cleaner_context_struct {

    // cleaner config
    mysql_cleaner_config_t *mysql_cleaner_config;

    // mysql connection
    mysql_context_t *mysql_context;

    // next check time (milliseconds)
    int64_t next_check_time;

} mysql_cleaner_context_t;

// static variables
static volatile int shutdown_flag = 0;
static mysql_cleaner_context_t *mysql_cleaner_context;
static pthread_t led_controller_thread;

// function declaration
void *do_start_mysql_cleaner(void *mysql_cleaner_config_void);
int64_t mysql_get_free_space();
static void on_shutdown();

int start_mysql_cleaner(mysql_cleaner_config_t *mysql_cleaner_config) {
    m_log(M_LOG_INFO, "Starting pthread for MySQL cleaner...");
    int pthread_create_ret = pthread_create(
        &led_controller_thread,
        NULL,
        do_start_mysql_cleaner,
        mysql_cleaner_config);

    if (pthread_create_ret != 0) {
        m_log(M_LOG_ERROR, "Failed to create pthread for MySQL cleaner.");
        return -1;
    }
    return 0;
}

static void on_shutdown() {
    m_log(M_LOG_INFO, "Joining thread of MySQL cleaner...");
    shutdown_flag = 1;
    pthread_join(led_controller_thread, NULL);
    m_log(M_LOG_INFO, "Thread of MySQL cleaner exited.");
}

void *do_start_mysql_cleaner(void *mysql_cleaner_config_void) {
    // cast
    mysql_cleaner_config_t *mysql_cleaner_config = (mysql_cleaner_config_t *) mysql_cleaner_config_void;

    // set pthread name
    pthread_setname_np(pthread_self(), "MYSQL_CLEANER");

    // add shutdown callback
    add_shutdown_callback(on_shutdown);

    // cleaner context
    mysql_cleaner_context_t *mysql_cleaner_context = (mysql_cleaner_context_t *) malloc(sizeof(mysql_cleaner_context_t));
    memset(mysql_cleaner_context, 0, sizeof(mysql_cleaner_context_t));
    mysql_cleaner_context->mysql_cleaner_config = mysql_cleaner_config;

    // initialize cleaner context
    int init_mysql_cleaner_context_ret = init_mysql_cleaner_context(mysql_cleaner_context, mysql_cleaner_config);
    if (init_mysql_cleaner_context_ret < 0) {
        dispose_mysql_cleaner_context(mysql_cleaner_context);
        free(mysql_cleaner_context);
        return (void *) -1;
    }

    while (shutdown_flag == 0) {
        // check every 10ms
        usleep(10000); 

        // check the time, note that we should also check whether 
        // the next schedule time too long from now since the time synchronization
        int64_t time_now_millis = get_time_millis();
        if (!(time_now_millis >= mysql_cleaner_context->next_check_time
        || mysql_cleaner_context->next_check_time - time_now_millis > mysql_cleaner_config->check_interval * 10)) {
            continue;
        }
        mysql_cleaner_context->next_check_time = time_now_millis + mysql_cleaner_config->check_interval;

        // check free space
        int64_t bytes_free = mysql_get_free_space();
        if (bytes_free < 0) {
            continue;
        }
        if (bytes_free > mysql_cleaner_context->mysql_cleaner_config->rolling_free_space) {
            m_log(M_LOG_INFO, "MySQL storage free space %d.%dGB > %dGB, we have enough space.",
                (int) (bytes_free >> 30), (int) (bytes_free >> 20) % 1024,
                mysql_cleaner_context->mysql_cleaner_config->rolling_free_space >> 30);
            continue;
        }

        // log
        m_log(M_LOG_INFO, "MySQL storage free space %d.%dGB < %dGB, deleting old-dated rows...",
            (int) (bytes_free >> 30), (int) (bytes_free >> 20) % 1024,
            mysql_cleaner_context->mysql_cleaner_config->rolling_free_space >> 30);

        // test connection
        int test_mysql_connection_ret = test_mysql_connection(mysql_cleaner_context->mysql_context, mysql_cleaner_config->mysql_config);
        if (test_mysql_connection_ret < 0) {
            m_log(M_LOG_ERROR, "Failed to test MySQL connection, sleep for 10 seconds.");
            sleep(10);
            if (shutdown_flag) {
                break;
            }
        }

        // do cleaning
        char *table_names[3] = {
            "oil_sensor_metrics",
            "collect_module_metrics",
            "public_info",
        };

        for(int i=0;i<3;i++) {
            char *table_name = table_names[i];

            char sql_buffer[512];
            memset(sql_buffer, 0, sizeof(sql_buffer));

            sprintf(sql_buffer, "select min(create_time) as min_time from %s", table_name);
            m_log(M_LOG_INFO, "Execute SQL to query the minimum create time: %s", sql_buffer);
            int mysql_query_ret = mysql_query(mysql_cleaner_context->mysql_context->mysql, sql_buffer);
            if (mysql_query_ret != 0) {
                m_log(M_LOG_ERROR, "Failed to execute query SQL '%s': %s", sql_buffer, mysql_error(mysql_cleaner_context->mysql_context->mysql));
                continue;
            }

            int64_t min_create_time = 0;
            MYSQL_RES *min_time_mysql_res = mysql_store_result(mysql_cleaner_context->mysql_context->mysql);
            if (mysql_num_rows(min_time_mysql_res)) {
                MYSQL_ROW row = mysql_fetch_row(min_time_mysql_res);
                char *min_create_time_str = row[0];
                if (min_create_time_str != NULL) {
                    min_create_time = atoll(min_create_time_str);
                }
            }
            mysql_free_result(min_time_mysql_res);

            if (min_create_time != 0L) {
                // delete 1 hours' rows per time
                int64_t clean_before_time = min_create_time + 1000LL * 3600LL;
                sprintf(sql_buffer, "delete from %s where create_time < %lld", table_name, clean_before_time);
                m_log(M_LOG_INFO, "Execute delete SQL to purge old data: %s", sql_buffer);
                int mysql_delete_ret = mysql_query(mysql_cleaner_context->mysql_context->mysql, sql_buffer);
                if (mysql_delete_ret != 0) {
                    m_log(M_LOG_ERROR, "Failed to execute delete SQL '%s': %s", sql_buffer, mysql_error(mysql_cleaner_context->mysql_context->mysql));
                    continue;
                }
                MYSQL_RES *delete_mysql_res = mysql_store_result(mysql_cleaner_context->mysql_context->mysql);
                mysql_free_result(delete_mysql_res);
            }
        }
    }

    // dispose resource
    dispose_mysql_cleaner_context(mysql_cleaner_context);
    free(mysql_cleaner_context);

    // return
    return (void *) 0;
}

int init_mysql_cleaner_context(mysql_cleaner_context_t *mysql_cleaner_context, mysql_cleaner_config_t *mysql_cleaner_config) {

    // mysql context
    m_log(M_LOG_INFO, "Initializing MySQL connection for MySQL cleaner");
    mysql_context_t *mysql_context = (mysql_context_t *)malloc(sizeof(mysql_context_t));
    memset(mysql_context, 0, sizeof(mysql_context_t));
    int init_mysql_connection_ret = init_mysql_connection(mysql_context, mysql_cleaner_config->mysql_config);
    if (init_mysql_connection_ret != 0) {
        m_log(M_LOG_ERROR, "Failed to initialize MySQL connection.");
        free(mysql_context);
        return -1;
    }
    mysql_cleaner_context->mysql_context = mysql_context;
    m_log(M_LOG_INFO, "Initialize MySQL connection successfully for MySQL cleaner");

    // return
    return 0;
}

int dispose_mysql_cleaner_context(mysql_cleaner_context_t *mysql_cleaner_context) {
    // dispose mysql context
    if (mysql_cleaner_context->mysql_context != NULL) {
        dispose_mysql_context(mysql_cleaner_context->mysql_context);
        free(mysql_cleaner_context->mysql_context);
        mysql_cleaner_context->mysql_context = NULL;
    }
}

int64_t mysql_get_free_space() {
    char *mysql_path = "/media/sdcard";
    struct statfs stat;
    int statfs_ret = statfs(mysql_path, &stat);
    if (statfs_ret < 0) {
        m_log(M_LOG_ERROR, "Failed to retrieve free space information of MySQL storage '%s', ret=%d", mysql_path, statfs_ret);
        return -1;
    }

    int64_t bytes_free = (int64_t) stat.f_bfree * (int64_t) stat.f_bsize;
    return bytes_free;
}

