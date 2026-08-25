#ifndef __LED_H
#define __LED_H 1

typedef enum sys_running_status_enum {
    SYS_RUNNING = 0,
    SYS_ERROR = 1,
} sys_running_status_t;

typedef enum comm_active_status_enum {
    COMM_INACTIVE = 0,
    COMM_ACTIVE = 1,
} comm_active_status_t;

typedef enum error_status_enum {
    STATUS_NORMAL = 0,
    STATUS_ERROR = 1,
} error_status_t;

typedef struct led_config_struct {
    // GPIO pin number of System Running Indicator
    int sys_running_led_gpio_pin;

    // GPIO pin number of Sensor Communication Error Indicator
    int sensor_comm_error_led_gpio_pin;

    // GPIO pin number of Storage/MySQL Error Indicator
    int error_led_gpio_pin;

} led_config_t;

int start_led_controller(led_config_t *led_config);

void led_set_public_packet_status(comm_active_status_t comm_active_status);
void led_set_sensor_error_status(int sensor_index, error_status_t modbus_error_status);
void led_set_mysql_error_status(error_status_t mysql_error_status);
void led_set_storage_error_status(error_status_t storage_error_status);

#endif