#ifndef __MYSQL_EXECUTOR_H

#define __MYSQL_EXECUTOR_H 1

#include <mysql.h>
#include <mysql-config/mysql-config.h>

typedef struct task_queue_struct {
    void **queue_elements;
    int queue_size;

    int front;
    int rear;

} task_queue_t;

typedef struct mysql_executor_config_struct {
    // mysql connection configs
    mysql_config_t *mysql_config;

    // ignore MySQL initialization failure
    int ignore_init_failure;

    // queue size
    int queue_size;

} mysql_executor_config_t;

typedef struct mysql_executor_context_struct {
    // config
    mysql_executor_config_t *mysql_executor_config;

    // mysql
    mysql_context_t *mysql_context;

    // queue
    task_queue_t *queue;
    pthread_mutex_t *queue_mutex;
    pthread_cond_t *queue_cond;
} mysql_executor_context_t;

int start_mysql_executor(mysql_executor_config_t *mysql_executor_config);
int execute_sql_async(char *sql);

#endif