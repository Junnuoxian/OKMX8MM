#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "log/log.h"
#include "gpio.h"

int gpio_set_direction(int pin, gpio_direction_t direction) {
    char gpio_direction_path[128];
    sprintf(gpio_direction_path, "/sys/class/gpio/gpio%d/direction", pin);

    // export it if gpio direction file exists?
    int access_ret = access(gpio_direction_path, F_OK);
    if (access_ret != 0) {
        char *gpio_export_path = "/sys/class/gpio/export";
        int gpio_export_fd = open(gpio_export_path, O_WRONLY);
        if (gpio_export_fd < 0) {
            m_log(M_LOG_ERROR, "Failed to open GPIO export file %s", gpio_export_path);
            return -1;
        }

        char data[8];
        sprintf(data, "%d\n", pin);

        int written = write(gpio_export_fd, data, strlen(data));
        close(gpio_export_fd);
        if (written <=0 ) {
            m_log(M_LOG_ERROR, "Failed to write GPIO export file %s, value: %s", gpio_export_path, data);
            return -1;
        }
    }

    // echo "out" > gpio direction
    char data[8];
    sprintf(data, direction == GPIO_DIRECTION_OUTPUT ? "out" : "in");

    int gpio_direction_fd = open(gpio_direction_path, O_RDWR);
    if (gpio_direction_fd < 0) {
        m_log(M_LOG_ERROR, "Failed to open GPIO direction file %s", gpio_direction_path);
        return -1;
    }

    int written = write(gpio_direction_fd, data, strlen(data));
    if (written <=0 ) {
        close(gpio_direction_fd);
        m_log(M_LOG_ERROR, "Failed to write GPIO direction file %s, value: %s, written: %d", gpio_direction_path, data, strlen(data));
        return -1;
    }
    close(gpio_direction_fd);
    return 0;
}

int gpio_set_state(int pin, gpio_state_t value) {
    char path[128];
    sprintf(path, "/sys/class/gpio/gpio%d/value", pin);
    
    int fd = open(path, O_RDWR);
    if (fd < 0) {
        m_log(M_LOG_ERROR, "Failed to open GPIO value file %s", path);
        return -1;
    }

    int written = write(fd, value ? "1" : "0", 1);
    if (written <=0 ) {
        m_log(M_LOG_ERROR, "Failed to write GPIO value file %s, value: %d", path, value);
        return -1;
    }

    close(fd);
    return 0;
}

int set_sensor_power_state(sensor_power_state_t state) {
    const int pins_length = 4;
    int pins[pins_length];
    pins[0]=117;
    pins[1]=118;
    pins[2]=119;
    pins[3]=120;

    int gpio_state = state == SENSOR_POWER_ON ? GPIO_STATE_HIGH : GPIO_STATE_LOW;
    for(int i=0; i<pins_length; i++) {
        gpio_set_direction(pins[i], GPIO_DIRECTION_OUTPUT);
        gpio_set_state(pins[i], gpio_state);
    }
    return 0;
}

