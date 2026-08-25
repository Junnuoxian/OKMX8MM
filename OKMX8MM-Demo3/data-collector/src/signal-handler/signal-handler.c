#include <stdlib.h>
#include <pthread.h>
#include <signal.h>

#include "log/log.h"

// static variables
static pthread_mutex_t mutex;

static int shutting_down = 0;
static void (*shutdown_callbacks[512])();
static int shutdown_callback_length = 0;

// function declaration
static int register_signal_handlers();
static void sigint_handler();

// implementation
int init_signal_handler() {
    // init mutex
    pthread_mutex_init(&mutex, NULL);

    // register signal handlers
    register_signal_handlers();
}

int add_shutdown_callback(void (*shutdown_callback)()) {
    int lock_ret = pthread_mutex_lock(&mutex);
    if (lock_ret != 0) {
        m_log(M_LOG_ERROR, "Failed to lock mutex of shutdown callbacks");
        return -1;
    }
    shutdown_callbacks[shutdown_callback_length++] = shutdown_callback;
    int unlock_ret = pthread_mutex_unlock(&mutex);
    if (unlock_ret != 0) {
        m_log(M_LOG_ERROR, "Failed to unlock mutex of shutdown callbacks");
        return -1;
    }
}

static int register_signal_handlers() {
    signal(SIGINT, sigint_handler);
}

void fire_sigint_handler() {
    sigint_handler();
}

static void sigint_handler() {
    shutting_down = 1;
    m_log(M_LOG_WARN, "Receive signal SIGINT, program will exit!");
    for (int i = shutdown_callback_length -1; i >= 0; i--) {
        void (*shutdown_callback)() = shutdown_callbacks[i];
        shutdown_callback();
    }

    exit(0);
}
