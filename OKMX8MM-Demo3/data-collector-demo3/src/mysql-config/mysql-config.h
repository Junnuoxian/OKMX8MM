#ifndef __MYSQL_CONFIG_H

#define __MYSQL_CONFIG_H 1

#define CONN_TYPE_SOCKET 0
#define CONN_TYPE_EMBEDDED 1

#include <mysql.h>

typedef struct mysql_config_struct {
    int conn_type;
    char *host;
    char *user;
    char *passwd;
    char *database;
    int port;
} mysql_config_t;

typedef struct mysql_context_struct {
    // mysql config
    mysql_config_t *mysql_config;

    // mysql connection
    MYSQL *mysql;
    int64_t mysql_create_time;
} mysql_context_t;

int test_mysql_connection(mysql_context_t *mysql_context, mysql_config_t *mysql_config);
int init_mysql_connection(mysql_context_t *mysql_context, mysql_config_t *mysql_config);
int init_mysql_library();
int dispose_mysql_context(mysql_context_t *mysql_context);

#endif