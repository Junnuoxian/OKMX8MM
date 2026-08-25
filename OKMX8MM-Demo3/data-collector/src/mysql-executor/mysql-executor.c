#include <stdio.h>
#include <string.h>
#include <mysql.h>
#include <pthread.h>

#include "log/log.h"
#include "util/util.h"
#include "signal-handler/signal-handler.h"
#include "led/led.h"
#include "mysql-executor.h"

// global variables
static volatile int shutdown_flag = 0;
static mysql_executor_context_t global_mysql_executor_context;
static pthread_t mysql_executor_thread;

// function declaration
static task_queue_t *create_task_queue(int queue_size);
static int dispose_task_queue(task_queue_t *queue);
static int is_queue_empty(task_queue_t *queue);
static int is_queue_full(task_queue_t *queue);
static int enqueue(task_queue_t *queue, void *element);
static void *dequeue(task_queue_t *queue);

static int init_mysql_executor_context(mysql_executor_context_t *mysql_executor_context);
static int dispose_mysql_executor_context(mysql_executor_context_t *mysql_executor_context);

static void *do_start_mysql_executor();
static void on_shutdown();
static int do_execute_sql(mysql_executor_context_t *mysql_executor_context, char *sql);

int start_mysql_executor(mysql_executor_config_t *mysql_executor_config) {
    // initizlie executor context
    mysql_executor_context_t *mysql_executor_context = &global_mysql_executor_context;
    memset(mysql_executor_context, 0, sizeof(mysql_executor_context_t));
    mysql_executor_context->mysql_executor_config = mysql_executor_config;

    int init_ret = init_mysql_executor_context(mysql_executor_context);
    if (init_ret < 0) {
        dispose_mysql_executor_context(mysql_executor_context);
        return -1;
    }

    // add shutdown callback
    add_shutdown_callback(on_shutdown);

    // start mysql executor pthread
    m_log(M_LOG_INFO, "Starting pthread for MySQL executor...");
    int pthread_create_ret = pthread_create(
        &mysql_executor_thread,
        NULL,
        do_start_mysql_executor,
        NULL);

    if (pthread_create_ret != 0) {
        m_log(M_LOG_ERROR, "Failed to create pthread for MySQL executor.");
        return -1;
    }
    return 0;
}

static void on_shutdown() {
    m_log(M_LOG_INFO, "Joining thread of MySQL executor...");
    
    shutdown_flag = 1;
    if (global_mysql_executor_context.queue_cond != NULL) {
        pthread_cond_signal(global_mysql_executor_context.queue_cond);
    }
    pthread_join(mysql_executor_thread, NULL);
    
    m_log(M_LOG_INFO, "Thread of MySQL executor exited.");
}

static int init_mysql_executor_context(mysql_executor_context_t *mysql_executor_context) {
    // mysql context
    m_log(M_LOG_INFO, "Initializing MySQL connection for MySQL executor");
    mysql_context_t *mysql_context = (mysql_context_t *)malloc(sizeof(mysql_context_t));
    memset(mysql_context, 0, sizeof(mysql_context_t));
    int init_mysql_connection_ret = init_mysql_connection(mysql_context, mysql_executor_context->mysql_executor_config->mysql_config);
    if (init_mysql_connection_ret != 0) {
        m_log(M_LOG_ERROR, "Failed to initialize MySQL connection.");
        if (!(mysql_executor_context->mysql_executor_config->ignore_init_failure)) {
            free(mysql_context);
            return -1;
        }
    }
    mysql_executor_context->mysql_context = mysql_context;
    if(mysql_executor_context->mysql_context->mysql != NULL) {
        m_log(M_LOG_INFO, "Initialize MySQL connection successfully for MySQL executor");
    }

    // queue
    mysql_executor_context->queue = create_task_queue(mysql_executor_context->mysql_executor_config->queue_size);

    mysql_executor_context->queue_mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    memset(mysql_executor_context->queue_mutex, 0, sizeof(pthread_mutex_t));
    int mutex_init_ret = pthread_mutex_init(mysql_executor_context->queue_mutex, NULL);
    if (mutex_init_ret != 0) {
        m_log(M_LOG_ERROR, "Failed to initialize mutext lock for queue");
        return -1;
    }

    mysql_executor_context->queue_cond = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
    memset(mysql_executor_context->queue_cond, 0, sizeof(pthread_cond_t));
    int cond_init_ret = pthread_cond_init(mysql_executor_context->queue_cond, NULL);
    if (cond_init_ret != 0) {
        m_log(M_LOG_ERROR, "Failed to initialize cond for queue");
        return -1;
    }

    // return
    return 0;
}

static int dispose_mysql_executor_context(mysql_executor_context_t *mysql_executor_context) {
    // dispose mysql context
    if (mysql_executor_context->mysql_context != NULL) {
        dispose_mysql_context(mysql_executor_context->mysql_context);
        free(mysql_executor_context->mysql_context);
        mysql_executor_context->mysql_context = NULL;
    }

    if (mysql_executor_context->queue != NULL) {
        dispose_task_queue(mysql_executor_context->queue);
        mysql_executor_context->queue = NULL;
    }

    if (mysql_executor_context->queue_mutex == NULL) {
        pthread_mutex_destroy((mysql_executor_context->queue_mutex));
        free(mysql_executor_context->queue_mutex);
        mysql_executor_context->queue_mutex = NULL;
    }

    if (mysql_executor_context->queue_cond == NULL) {
        pthread_cond_destroy(mysql_executor_context->queue_cond);
        free(mysql_executor_context->queue_cond);
        mysql_executor_context->queue_cond = NULL;
    }
}

static void *do_start_mysql_executor() {
    // set pthread name
    pthread_setname_np(pthread_self(), "MYSQL_EXECUTOR");

    // context
    mysql_executor_context_t *mysql_executor_context = &global_mysql_executor_context;

    // loop
    while (shutdown_flag == 0) {
        // sql to execute
        char *sql = NULL;

        // lock and dequeue
        int mutex_lock_ret = pthread_mutex_lock(mysql_executor_context->queue_mutex);
        if (mutex_lock_ret != 0) {
            m_log(M_LOG_ERROR, "Failed to acquire lock of task queue");
            usleep(100);  // sleep for 100ms
            continue;
        }

        if (is_queue_empty(mysql_executor_context->queue)) {
            pthread_cond_wait(mysql_executor_context->queue_cond, mysql_executor_context->queue_mutex);
        }
        if (!shutdown_flag && !is_queue_empty(mysql_executor_context->queue)) {
            sql = dequeue(mysql_executor_context->queue);
        }

        int mutex_unlock_ret = pthread_mutex_unlock(mysql_executor_context->queue_mutex);
        if (mutex_unlock_ret != 0) {
            m_log(M_LOG_ERROR, "Failed to release lock of task queue");
        }

        if (sql != NULL) {
            int execute_sql_ret = do_execute_sql(mysql_executor_context, sql);
            led_set_mysql_error_status(execute_sql_ret == 0 ? STATUS_NORMAL : STATUS_ERROR);
            free(sql);
        }
    }

    // return
    return (void *) 0;
}

int execute_sql_async(char *sql) {
    mysql_executor_context_t *mysql_executor_context = &global_mysql_executor_context;
    if (shutdown_flag || mysql_executor_context->mysql_context == NULL) {
        return -1;
    }

    // copy the sql
    char *copy_sql = (char *)malloc(strlen(sql) + 8);
    strcpy(copy_sql, sql);

    // lock and enqueue
    int mutex_lock_ret = pthread_mutex_lock(mysql_executor_context->queue_mutex);
    if (mutex_lock_ret != 0) {
        m_log(M_LOG_ERROR, "Failed to acquire lock of task queue");
        return -1;
    }

    int ret = 0;
    if (!is_queue_full(mysql_executor_context->queue)) {
        ret = enqueue(mysql_executor_context->queue, copy_sql);
    } else {
        ret = -1;
        m_log(M_LOG_ERROR, "Queue is full and will abandon this SQL '%s'", copy_sql);
        free(copy_sql);
    }

    int mutex_unlock_ret = pthread_mutex_unlock(mysql_executor_context->queue_mutex);
    if (mutex_unlock_ret != 0) {
        m_log(M_LOG_ERROR, "Failed to release lock of task queue");
    }

    // wake up the consumer
    pthread_cond_signal(mysql_executor_context->queue_cond);
    return ret;
}

static int do_execute_sql(mysql_executor_context_t *mysql_executor_context, char *sql) {
    // test connection
    int test_mysql_connection_ret = test_mysql_connection(mysql_executor_context->mysql_context, mysql_executor_context->mysql_context->mysql_config);
    if (test_mysql_connection_ret < 0) {
        m_log(M_LOG_ERROR, "Failed to test MySQL connection, sleep for 10 seconds.");
        sleep(10);
        if (shutdown_flag) {
            return -1;
        }
    }
    // metrics the wasted time
    int start_time_millis = get_time_millis();

    // do execute SQL
    m_log(M_LOG_INFO, "Execute SQL: %s", sql);
    int mysql_real_query_ret = mysql_query(mysql_executor_context->mysql_context->mysql, sql);
    if (mysql_real_query_ret != 0) {
        m_log(M_LOG_ERROR, "Failed to execute SQL '%s': %s", sql, mysql_error(mysql_executor_context->mysql_context->mysql));
        return -1;
    }
    MYSQL_RES *mysql_res = mysql_store_result(mysql_executor_context->mysql_context->mysql);
    mysql_free_result(mysql_res);

    // log the wasted time
    m_log(M_LOG_INFO, "Execute SQL complete, wasted(ms): %d", get_time_millis() - start_time_millis);

    return 0;
}

static task_queue_t *create_task_queue(int queue_size) {
    // create and initialize queue
    task_queue_t *queue = (task_queue_t *)malloc(sizeof(task_queue_t));
    memset(queue, 0, sizeof(task_queue_t));

    queue->queue_size = queue_size;
    queue->queue_elements = (void **)malloc(sizeof(void *) * queue_size);

    // return;
    return queue;
}

static int dispose_task_queue(task_queue_t *queue) {
    free(queue->queue_elements);
    free(queue);
}

static int is_queue_empty(task_queue_t *queue) {
    if (queue->front == queue->rear) {
        return 1;
    }
    return 0;
}

static int is_queue_full(task_queue_t *queue) {
    int front_next = (queue->front + 1) % queue->queue_size;
    if (front_next == queue->rear) {
        return 1;
    }
    return 0;
}

static int enqueue(task_queue_t *queue, void *element) {
    if (is_queue_full(queue)) {
        return -1;
    }
    queue->queue_elements[queue->front] = element;
    queue->front = (queue->front + 1) % queue->queue_size;
    return 0;
}

static void *dequeue(task_queue_t *queue) {
    if (is_queue_empty(queue)) {
        return NULL;
    }

    void *element = queue->queue_elements[queue->rear];
    queue->rear = (queue->rear + 1) % queue->queue_size;
    return element;
}
