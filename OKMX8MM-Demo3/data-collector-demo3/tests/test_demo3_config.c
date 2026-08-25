#include <string.h>

#include "config/demo3_config.h"

static int expect_string(const char *actual, const char *expected)
{
    return strcmp(actual, expected) == 0 ? 0 : 1;
}

int main(void)
{
    demo3_config_t config;

    demo3_config_init(&config);
    if (config.baudrate != 9600 || config.sample_period_ms != 1000) {
        return 1;
    }
    if (expect_string(config.serial_device, "") != 0) {
        return 2;
    }
    if (expect_string(config.source, "modbus") != 0 ||
        config.rpmsg_poll_timeout_ms != 1000 || config.can_enabled != 0 ||
        config.can_id_base != 0x300) {
        return 3;
    }

    if (demo3_config_apply_line(&config, "serial_device = /dev/demo3-uart") != 0) {
        return 4;
    }
    if (demo3_config_apply_line(&config, "baudrate=115200") != 0) {
        return 5;
    }
    if (demo3_config_apply_line(&config, "sample_period_ms=200") != 0) {
        return 6;
    }
    if (demo3_config_apply_line(&config, "mqtt_enabled=true") != 0) {
        return 7;
    }
    if (demo3_config_apply_line(&config, "public_udp_port=7100") != 0) {
        return 8;
    }
    if (demo3_config_apply_line(&config, "source=rpmsg") != 0 ||
        demo3_config_apply_line(&config, "rpmsg_device=/dev/rpmsg_demo3") != 0 ||
        demo3_config_apply_line(&config, "rpmsg_poll_timeout_ms=250") != 0 ||
        demo3_config_apply_line(&config, "can_enabled=true") != 0 ||
        demo3_config_apply_line(&config, "can_interface=can0") != 0 ||
        demo3_config_apply_line(&config, "can_id_base=768") != 0) {
        return 9;
    }
    if (expect_string(config.serial_device, "/dev/demo3-uart") != 0 ||
        config.baudrate != 115200 ||
        config.sample_period_ms != 200 ||
        config.mqtt_enabled != 1 ||
        config.public_udp_port != 7100 ||
        expect_string(config.source, "rpmsg") != 0 ||
        expect_string(config.rpmsg_device, "/dev/rpmsg_demo3") != 0 ||
        config.rpmsg_poll_timeout_ms != 250 || config.can_enabled != 1 ||
        expect_string(config.can_interface, "can0") != 0 ||
        config.can_id_base != 768) {
        return 10;
    }

    return 0;
}
