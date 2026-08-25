#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <mysql.h>

#include "mysql-config.h"
#include "log/log.h"

#define CONNECITON_EXPIRE_TIME (1000 * 60 * 10)

int test_mysql_connection(mysql_context_t *mysql_context, mysql_config_t *mysql_config) {
    // check expiration
    int64_t time_now_millis = get_time_millis();
    if (time_now_millis - mysql_context->mysql_create_time > CONNECITON_EXPIRE_TIME || mysql_context->mysql == NULL) {
        m_log(M_LOG_INFO, "Connection opened over %d minutes, reinitialize MySQL connection.", CONNECITON_EXPIRE_TIME / 1000 / 60);
        int init_mysql_connection_ret = init_mysql_connection(mysql_context, mysql_config);
        if (init_mysql_connection_ret < 0) {
            m_log(M_LOG_ERROR, "Failed to reinitialize MySQL connection: %s", mysql_error(mysql_context->mysql));
            return -1;
        }
        m_log(M_LOG_INFO, "Reinitialize MySQL connection successfully.");
    }

    // execute test sql
    for (int i = 0; i < 2; i++) {
        char *test_sql = "SELECT 1=1";
        int mysql_query_ret = mysql_query(mysql_context->mysql, test_sql);
        if (mysql_query_ret != 0) {
            if (i >= 1) {
                m_log(M_LOG_INFO, "Failed to execute test SQL '%s': %s", test_sql, mysql_error(mysql_context->mysql));
                return -1;
            }

            m_log(M_LOG_INFO, "Failed to execute test SQL '%s' and will reinitialize MySQL connection: %s", test_sql, mysql_error(mysql_context->mysql));
            int init_mysql_connection_ret = init_mysql_connection(mysql_context, mysql_config);
            if (init_mysql_connection_ret < 0) {
                m_log(M_LOG_ERROR, "Failed to reinitialize MySQL connection: %s", mysql_error(mysql_context->mysql));
                return -1;
            }
            m_log(M_LOG_INFO, "Reinitialize MySQL connection successfully.");
        } else {
            MYSQL_RES *mysql_res = mysql_store_result(mysql_context->mysql);
            mysql_free_result(mysql_res);
            break;
        }
    }

    return 0;
}

int init_mysql_library() {
    // init mysql library
    m_log(M_LOG_INFO, "Initializing MySQL library.");
    int mysql_library_init_ret = mysql_library_init(0, NULL, NULL);
    if (mysql_library_init_ret != 0) {
        m_log(M_LOG_ERROR, "Failed to initialize MySQL library.");
        return -1;
    }
    m_log(M_LOG_INFO, "Initialize MySQL library successfully.");
    return 0;
}

int init_mysql_connection(mysql_context_t *mysql_context, mysql_config_t *mysql_config) {
    // close existed mysql instance
    if (mysql_context->mysql != NULL) {
        m_log(M_LOG_INFO, "Close mysql connection of %s:%d", mysql_config->host, mysql_config->port);
        mysql_close(mysql_context->mysql);
        mysql_context->mysql = NULL;
    }

    // set mysql config
    mysql_context->mysql_config = mysql_config;

    // init mysql connection
    MYSQL *mysql = (MYSQL *)malloc(sizeof(MYSQL));
    memset(mysql, 0, sizeof(MYSQL));
    mysql_context->mysql = mysql;

    if (mysql_init(mysql) == NULL) {
        m_log(M_LOG_ERROR, "Failed to initialize MySQL handler: '%s'", mysql_error(mysql));
        return -1;
    }
    m_log(M_LOG_INFO, "Initialize MySQL handler successfully.");

    m_log(M_LOG_INFO, "Connecting to MySQL server '%s:%d'", mysql_config->host, mysql_config->port);
    MYSQL *mysql_real_connect_ret = mysql_real_connect(
        mysql,
        mysql_config->host,
        mysql_config->user,
        mysql_config->passwd,
        mysql_config->database,
        mysql_config->port,
        NULL,
        0);
    if (mysql_real_connect_ret == NULL) {
        m_log(M_LOG_ERROR, "Failed to connect to MySQL server '%s:%d', error: %s", mysql_config->host, mysql_config->port, mysql_error(mysql));
        return -1;
    }
    m_log(M_LOG_INFO, "Connect to MySQL server '%s:%d' successfully.", mysql_config->host, mysql_config->port);

    int mysql_set_character_set_ret = mysql_set_character_set(mysql, "utf8mb4");
    if (mysql_set_character_set_ret != 0) {
        m_log(M_LOG_ERROR, "Failed to set character set to utf8mb4.");
        return -1;
    }
    m_log(M_LOG_INFO, "Set character set to utf8mb4.");

    mysql_context->mysql = mysql;
    mysql_context->mysql_create_time = get_time_millis();
    return 0;
}

int dispose_mysql_context(mysql_context_t *mysql_context) {
    if (mysql_context == NULL) {
        return 0;
    }
    if (mysql_context->mysql != 0) {
        mysql_close(mysql_context->mysql);
        mysql_context->mysql = NULL;
        m_log(M_LOG_INFO, "Close MySQL connection.");
    }
}