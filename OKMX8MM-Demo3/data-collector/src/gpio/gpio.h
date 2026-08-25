#ifndef __GPIO_H
#define __GPIO_H 1


typedef enum gpio_state_enum {
    GPIO_STATE_LOW = 0,
    GPIO_STATE_HIGH = 1,
} gpio_state_t;

typedef enum gpio_direction_enum {
    GPIO_DIRECTION_OUTPUT = 0,
    GPIO_DIRECTION_INPUT = 1,
} gpio_direction_t;

int gpio_set_direction(int pin, gpio_direction_t direction);
int gpio_set_state(int pin, gpio_state_t value);


typedef enum sensor_power_state_enum {
    SENSOR_POWER_ON = 0,
    SENSOR_POWER_OFF = 1,
} sensor_power_state_t;

int set_sensor_power_state(sensor_power_state_t state);

#endif
