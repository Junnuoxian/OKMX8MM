#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#include "util/util.h"
#include "log/log.h"
#include "gpio/gpio.h"
#include "led.h"

// type declaration

typedef enum led_state_enum {
    LED_ON = 0,
    LED_OFF = 1,
} led_state_t;

typedef struct led_context_struct {

    // config
    led_config_t *led_config;

    // runtime
    volatile int64_t ticks;

    // system running status
    volatile sys_running_status_t system_running_status;
    volatile comm_active_status_t public_packet_status;
    led_state_t system_running_led_state;
    int64_t system_running_last_flicker_ticks;
    int64_t system_running_last_reset_ticks;
    int64_t public_packet_last_reset_ticks;

    // sensor modbus communication error
    volatile int error_sensor_flags;
    led_state_t error_sensor_led_state;
    int64_t error_sensor_last_flicker_ticks;
    int64_t error_sensor_last_reset_ticks;

    // error status
    volatile error_status_t mysql_error_status;
    volatile error_status_t storage_error_status;

    led_state_t error_led_state;
    int64_t error_last_flicker_ticks;
    int64_t error_last_reset_ticks;

} led_context_t;

// static variables
static volatile int shutdown_flag = 0;
static led_context_t *led_context;
static pthread_t led_controller_thread;

// function declaration
static void on_shutdown();
static void *do_start_led_controller();
static int led_initializing();
static int led_set_output(int pin);
static int led_set_state(int pin, led_state_t value);
static led_state_t flip_led_state(led_state_t led_state);

int start_led_controller(led_config_t *led_config) {
    // init led context
    led_context = (led_context_t *) malloc(sizeof(led_context_t));
    memset(led_context, 0, sizeof(led_context_t));
    led_context->led_config = led_config;
    led_context->system_running_status = SYS_RUNNING;
    led_context->public_packet_status = COMM_ACTIVE;

    m_log(M_LOG_INFO, "Starting pthread for LED controller...");
    int pthread_create_ret = pthread_create(
        &led_controller_thread,
        NULL,
        do_start_led_controller,
        NULL);

    if (pthread_create_ret != 0) {
        m_log(M_LOG_ERROR, "Failed to create pthread for LED controller.");
        return -1;
    }
    return 0;
}

static void on_shutdown() {
    m_log(M_LOG_INFO, "Joining thread of LED controller...");
    shutdown_flag = 1;
    pthread_join(led_controller_thread, NULL);
    m_log(M_LOG_INFO, "Thread of LED controller exited.");
}

static void *do_start_led_controller() {
    // set pthread name
    pthread_setname_np(pthread_self(), "LED_FLICKER");

    int system_running_flip_interval = 1000;
    int system_running_reset_interval = 2000;
    int system_running_flips = 0;
    int public_packet_reset_interval = 5000;

    int error_sensor_flip_interval = 200;
    int error_sensor_flips = 0;
    int error_sensor_index = 0;
    int error_sensor_reset_interval = 3000;

    int error_flip_interval = 250;
    int error_flips = 0;
    int error_reset_interval = 3000;

    const int sys_running_led_gpio_pin = led_context->led_config->sys_running_led_gpio_pin;
    const int error_sensor_led_gpio_pin = led_context->led_config->sensor_comm_error_led_gpio_pin;
    const int error_led_gpio_pin = led_context->led_config->error_led_gpio_pin;

    led_set_output(sys_running_led_gpio_pin);
    led_set_output(error_sensor_led_gpio_pin);
    led_set_output(error_led_gpio_pin);

    int64_t ticks = 0;
    while(!shutdown_flag) {
        int64_t ticks_now = ticks;

        // system running status
        int64_t system_running_next_reset_ticks = led_context->system_running_last_reset_ticks + system_running_reset_interval;
        if (ticks_now >= system_running_next_reset_ticks) {
            led_context->system_running_last_reset_ticks = ticks_now;
            system_running_flips = 0;
        }

        int64_t public_packet_next_reset_ticks = led_context->public_packet_last_reset_ticks + public_packet_reset_interval;
        if (ticks_now >= public_packet_next_reset_ticks) {
            led_context->public_packet_last_reset_ticks = ticks_now;
            led_context->public_packet_status = COMM_INACTIVE;
        }

        int system_running_flips_max = 1;
        if (led_context->public_packet_status == COMM_ACTIVE) {
            system_running_flip_interval = 1000;
            system_running_flips_max = 1;
        } else {
            system_running_flip_interval = 200;
            system_running_flips_max = 3;
        }

        int64_t sys_running_next_flicker_ticks = led_context->system_running_last_flicker_ticks + system_running_flip_interval;
        if (ticks_now >= sys_running_next_flicker_ticks) {
            led_context->system_running_last_flicker_ticks = ticks_now;
            led_context->system_running_led_state = system_running_flips < system_running_flips_max ? flip_led_state(led_context->system_running_led_state) : LED_OFF;
            led_set_state(sys_running_led_gpio_pin, led_context->system_running_led_state);
            if (led_context->system_running_led_state == LED_ON) {
                system_running_flips ++;
            }
        }

        // error sensor index
        while(led_context->error_sensor_flags) {
            int sensor_error = led_context->error_sensor_flags & (1 << error_sensor_index);
            if (!sensor_error) {
                error_sensor_index ++;
                if (error_sensor_index >= 8) {
                    error_sensor_index = 0;
                }
            } else {
                break;
            }
        }

        if (led_context->error_sensor_flags) {
            int error_sensor_flips_max = error_sensor_index + 1;
            int64_t error_sensor_next_reset_ticks = led_context->error_sensor_last_reset_ticks + error_sensor_reset_interval;
            if (ticks_now >= error_sensor_next_reset_ticks) {
                led_context->error_sensor_last_reset_ticks = ticks_now;
                error_sensor_flips = 0;
                error_sensor_index ++;
            }

            int64_t error_sensor_next_flicker_ticks = led_context->error_sensor_last_flicker_ticks + error_sensor_flip_interval;
            if (ticks_now >= error_sensor_next_flicker_ticks) {
                led_context->error_sensor_led_state = error_sensor_flips < error_sensor_flips_max ? flip_led_state(led_context->error_sensor_led_state) : LED_OFF;
                led_context->error_sensor_last_flicker_ticks = ticks_now;
                led_set_state(error_sensor_led_gpio_pin, led_context->error_sensor_led_state);
                if (led_context->error_sensor_led_state == LED_ON) {
                    error_sensor_flips ++;
                }
            }
        } else {
            led_context->error_sensor_led_state = LED_OFF;
            led_set_state(error_sensor_led_gpio_pin, led_context->error_sensor_led_state);
            error_sensor_flips = 0;
        }

        // error
        int error_status = (led_context->storage_error_status << 0) | (led_context->mysql_error_status << 1);
        if (error_status == 0) {
            led_context->error_led_state = LED_OFF;
            led_set_state(error_led_gpio_pin, led_context->error_led_state);
            error_flips = 0;
        } else {
            int64_t error_next_reset_ticks = led_context->error_last_reset_ticks + error_reset_interval;
            if (ticks_now >= error_next_reset_ticks) {
                led_context->error_last_reset_ticks = ticks_now;
                error_flips = 0;
            }

            int64_t error_next_flicker_ticks = led_context->error_last_flicker_ticks + error_flip_interval;
            if (ticks_now >= error_next_flicker_ticks) {
                led_context->error_led_state = error_flips < error_status ? flip_led_state(led_context->error_led_state) : LED_OFF;
                led_context->error_last_flicker_ticks = ticks_now;
                led_set_state(error_led_gpio_pin, led_context->error_led_state);
                if (led_context->error_led_state == LED_ON) {
                    error_flips ++;
                }
            }
        }

        // sleep
        usleep(50 * 1000);
        ticks += 50;
        led_context->ticks = ticks;
    }

    return (void *) 0;
}

void led_set_public_packet_status(comm_active_status_t comm_active_status) {
    if (led_initializing()) {
        return;
    }
    led_context->public_packet_status = comm_active_status;
    if (comm_active_status == COMM_ACTIVE) {
        led_context->public_packet_last_reset_ticks = led_context->ticks;
    }
}

void led_set_sensor_error_status(int sensor_index, error_status_t modbus_error_status) {
    if (led_initializing()) {
        return;
    }
    led_context->error_sensor_flags = (led_context->error_sensor_flags & (~(1 << sensor_index))) | (modbus_error_status << sensor_index);
}

void led_set_mysql_error_status(error_status_t mysql_error_status) {
    if (led_initializing()) {
        return;
    }
    led_context->mysql_error_status = mysql_error_status;
}

void led_set_storage_error_status(error_status_t storage_error_status) {
    if (led_initializing()) {
        return;
    }
    led_context->storage_error_status = storage_error_status;
}

int led_initializing() {
    if (led_context == NULL) {
        return 1;
    }
    return 0;
}

static int led_set_output(int pin) {
    return gpio_set_direction(pin, GPIO_DIRECTION_OUTPUT);
}

static int led_set_state(int pin, led_state_t state) {
    return gpio_set_state(pin, state == LED_OFF ? GPIO_STATE_HIGH : GPIO_STATE_LOW);
}

static led_state_t flip_led_state(led_state_t led_state) {
    return led_state == LED_ON ? LED_OFF : LED_ON;
}
