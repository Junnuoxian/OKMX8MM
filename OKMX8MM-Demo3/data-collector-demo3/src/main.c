#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <pthread.h>
#include <linux/can.h>

#include "log/log.h"
#include "signal-handler/signal-handler.h"
#include "mysql-config/mysql-config.h"
#include "mysql-cleaner/mysql-cleaner.h"
#include "mysql-executor/mysql-executor.h"
#include "modbus-rtu/modbus-rtu.h"
#include "public/public-udp-collector.h"
#include "file-storage/file-storage.h"
#include "bridge/demo3_rpmsg_service.h"
#include "ota/demo3_ota.h"
#include "led/led.h"
#include "gpio/gpio.h"
#include "config/demo3_config.h"
#include "main.h"

static speed_t demo3_baudrate_to_speed(int baudrate)
{
    switch (baudrate) {
        case 9600: return B9600;
        case 19200: return B19200;
        case 38400: return B38400;
        case 57600: return B57600;
        case 115200: return B115200;
        default: return (speed_t)0;
    }
}

static int demo3_prepare_parent_directory(const char *path)
{
    char parent[DEMO3_CONFIG_TEXT_LENGTH];
    char *separator;

    if (path == 0 || strlen(path) >= sizeof(parent)) {
        return -1;
    }
    (void)strcpy(parent, path);
    separator = strrchr(parent, '/');
    if (separator == 0) {
        return 0;
    }
    *separator = '\0';
    if (parent[0] == '\0' || strcmp(parent, ".") == 0) {
        return 0;
    }
    return mkdirs(parent);
}

int main(int argc, char **args) {
    demo3_config_t demo3_config;
    const char *config_path;

    (void)argc;
    (void)args;
    demo3_config_init(&demo3_config);
    config_path = getenv("DEMO3_CONFIG");
    if (config_path != NULL) {
        int config_ret = demo3_config_load(&demo3_config, config_path);
        if (config_ret != 0) {
            fprintf(stderr, "Failed to load Demo3 config '%s', ret=%d\n", config_path, config_ret);
            return -1;
        }
    }

    printf("Starting ...\n");

    // signal handler
    init_signal_handler();

    // log configuration
    m_log_config_t m_log_config;
    m_log_config.log_level = M_LOG_INFO;
    m_log_config.log_file_name_prefix = demo3_config.log_file_prefix;
    m_log_config.rolling_count = 5;                    // 5 files maximum
    m_log_config.rolling_file_size = 5 * 1024 * 1024;  // 5MB per one maximum
    int m_log_init_ret = m_log_init(&m_log_config);
    if (m_log_init_ret < 0) {
        printf("Failed to initialize logging.\n");
        return -1;
    }

    // log version information
    m_log(M_LOG_INFO, "%s (version: %s, build: %s %s, commit: %s)", ABOUT_NAME, ABOUT_VERSION, __DATE__, __TIME__, ABOUT_COMMIT);

    m_log(M_LOG_INFO, "Network setup is provided by the OKMX8MM system configuration.");

    int ota_staged = 0;
    if (demo3_config.status_enabled &&
        demo3_prepare_parent_directory(demo3_config.status_path) != 0) {
        m_log(M_LOG_WARN, "Runtime status path is unavailable: %s",
              demo3_config.status_path);
        demo3_config.status_enabled = 0;
    }
    if (demo3_config.ota_enabled) {
        if (demo3_config.ota_package_path[0] == '\0') {
            m_log(M_LOG_WARN, "OTA is enabled but ota_package_path is empty.");
        } else if (demo3_prepare_parent_directory(demo3_config.ota_staging_path) != 0 ||
                   demo3_prepare_parent_directory(demo3_config.ota_reboot_marker_path) != 0 ||
                   demo3_ota_stage_package(demo3_config.ota_package_path,
                                           demo3_config.ota_staging_path) != 0 ||
                   demo3_ota_write_reboot_marker(
                       demo3_config.ota_reboot_marker_path,
                       demo3_config.ota_staging_path) != 0) {
            m_log(M_LOG_ERROR, "OTA package staging failed; collector continues.");
        } else {
            ota_staged = 1;
            m_log(M_LOG_INFO,
                  "OTA package staged; reboot is required for bootloader handling.");
        }
    }

    if (strcmp(demo3_config.source, "modbus") != 0 &&
        strcmp(demo3_config.source, "rpmsg") != 0) {
        m_log(M_LOG_ERROR, "Unsupported data source '%s'", demo3_config.source);
        return -1;
    }

    if (demo3_config.sensor_power_enabled) {
        m_log(M_LOG_INFO, "Power cycling sensor supply ...");
        set_sensor_power_state(SENSOR_POWER_OFF);
        sleep(5);
        set_sensor_power_state(SENSOR_POWER_ON);
        sleep(4);
    }

    // LED indicator controller
    led_config_t led_config;
    if (demo3_config.led_enabled) {
        led_config.sys_running_led_gpio_pin = demo3_config.led_running_pin;
        led_config.sensor_comm_error_led_gpio_pin = demo3_config.led_sensor_error_pin;
        led_config.error_led_gpio_pin = demo3_config.led_error_pin;
        start_led_controller(&led_config);
    }

    // init MySQL library
    int init_mysql_library_ret = init_mysql_library();
    if (init_mysql_library_ret != 0) {
        m_log(M_LOG_ERROR, "Failed to initialize MySQL library.");
        exit(-1);
    }

    // mysql configuration
    mysql_config_t mysql_config;
    mysql_config.conn_type = CONN_TYPE_SOCKET;
    mysql_config.host = demo3_config.mysql_host;
    mysql_config.port = demo3_config.mysql_port;
    mysql_config.user = demo3_config.mysql_user;
    mysql_config.passwd = demo3_config.mysql_password;
    mysql_config.database = demo3_config.mysql_database;

    // mysql cleaner configuration
    mysql_cleaner_config_t mysql_cleaner_config;
    mysql_cleaner_config.mysql_config = &mysql_config;
    mysql_cleaner_config.check_interval = 1000 * 60;  // 1 minute per time
    mysql_cleaner_config.rolling_free_space = ((int64_t) 5LL * 1024LL * 1024LL * 1024LL);

    // mysql async executor configuration
    mysql_executor_config_t mysql_executor_config;
    mysql_executor_config.mysql_config = &mysql_config;
    mysql_executor_config.ignore_init_failure = 1; // ignore the MySQL failure since it is weak
    mysql_executor_config.queue_size = 1000;

    // modbus-rtu configuration
    modbus_rtu_t modbus_rtu;
    modbus_rtu.mysql_config = &mysql_config;
    modbus_rtu.uart_name = demo3_config.serial_device;
    modbus_rtu.bps = demo3_baudrate_to_speed(demo3_config.baudrate);
    modbus_rtu.start_bits = demo3_config.start_bits;
    modbus_rtu.data_bits = demo3_config.data_bits;
    modbus_rtu.stop_bits = demo3_config.stop_bits;
    modbus_rtu.parity_bits = demo3_config.parity_bits;

    if (strcmp(demo3_config.source, "modbus") == 0 &&
        (modbus_rtu.uart_name[0] == '\0' || modbus_rtu.bps == (speed_t)0)) {
        m_log(M_LOG_ERROR, "A valid serial_device and baudrate are required.");
        return -1;
    }

    // publish interval
    modbus_rtu.publish_interval = demo3_config.sample_period_ms;

    // file storage
    const int FILE_STORAGE_CONFIG_COUNT = 2;
    file_storage_config_t file_storage_configs[FILE_STORAGE_CONFIG_COUNT];
    file_storage_configs[0].file_name = demo3_config.storage_file_name;
    file_storage_configs[0].path = demo3_config.local_storage_path;
    file_storage_configs[0].compress = demo3_config.storage_compress;
    file_storage_configs[0].rolling_free_space =  500LL * 1024LL * 1024LL; // 500MB
    // SD card
    file_storage_configs[1].file_name = demo3_config.storage_file_name;
    file_storage_configs[1].path = demo3_config.sd_storage_path;
    file_storage_configs[1].compress = 0;
    file_storage_configs[1].rolling_free_space = 4LL * 1024LL * 1024LL * 1024LL; // 4GB

    modbus_rtu.file_storage_configs = file_storage_configs;
    modbus_rtu.file_storage_config_count = FILE_STORAGE_CONFIG_COUNT;
    
    // STM32F407 gateway exposed through one Modbus slave.
    const int MODBUS_RTU_DEVICE_COUNT = 1;
    modbus_rtu_device_t *modbus_rtu_devices[MODBUS_RTU_DEVICE_COUNT];
    modbus_rtu_device_t demo3_gateway_device;
    memset(&demo3_gateway_device, 0, sizeof(demo3_gateway_device));
    demo3_gateway_device.device_type = DEVICE_TYPE_DEMO3_GATEWAY;
    demo3_gateway_device.device_name = "STM32F407 Gateway";
    demo3_gateway_device.device_address = 0x01;
    demo3_gateway_device.collect_interval = demo3_config.sample_period_ms;
    modbus_rtu_devices[0] = &demo3_gateway_device;
    modbus_rtu.devices = modbus_rtu_devices;
    modbus_rtu.device_count = MODBUS_RTU_DEVICE_COUNT;

    // public packets collector configuration
    public_udp_config_t public_udp_config;
    public_udp_config.mysql_config = &mysql_config;
    public_udp_config.udp_host = demo3_config.public_udp_host;
    public_udp_config.udp_port = demo3_config.public_udp_port;

    int has_error = 0;

    // start and init MySQL executor
    if (!has_error) {
        int start_mysql_executor_ret = start_mysql_executor(&mysql_executor_config);
        if (start_mysql_executor_ret != 0) {
            has_error = 1;
        }
    }

    // start public information collector
    if (!has_error) {
        int start_public_udp_collector_ret = start_public_udp_collector(&public_udp_config);
        if (start_public_udp_collector_ret != 0) {
            has_error = 1;
        }
    }

    // start the selected data source
    if (!has_error) {
        if (strcmp(demo3_config.source, "rpmsg") == 0) {
            demo3_rpmsg_service_config_t rpmsg_config;
            rpmsg_config.device_path = demo3_config.rpmsg_device;
            rpmsg_config.local_storage_path = demo3_config.local_storage_path;
            rpmsg_config.sd_storage_path = demo3_config.sd_storage_path;
            rpmsg_config.storage_file_name = demo3_config.storage_file_name;
            rpmsg_config.storage_compress = demo3_config.storage_compress;
            rpmsg_config.poll_timeout_ms = demo3_config.rpmsg_poll_timeout_ms;
            rpmsg_config.can_enabled = demo3_config.can_enabled;
            rpmsg_config.can_interface = demo3_config.can_interface;
            rpmsg_config.can_id_base = (uint32_t)demo3_config.can_id_base;
            rpmsg_config.mqtt_enabled = demo3_config.mqtt_enabled;
            rpmsg_config.mqtt_broker = demo3_config.mqtt_broker;
            rpmsg_config.mqtt_port = demo3_config.mqtt_port;
            rpmsg_config.mqtt_topic = demo3_config.mqtt_topic;
            rpmsg_config.mqtt_client_id = demo3_config.mqtt_client_id;
            rpmsg_config.status_enabled = demo3_config.status_enabled;
            rpmsg_config.status_path = demo3_config.status_path;
            rpmsg_config.ota_staged = ota_staged;
            if (start_demo3_rpmsg_collector(&rpmsg_config) != 0) {
                has_error = 1;
            }
        } else {
            int start_modbus_collector_ret = start_modbus_collector(&modbus_rtu);
            if (start_modbus_collector_ret != 0) {
                has_error = 1;
            }
        }
    }

    // start mysql data cleaner
    if (!has_error) {
        /*
        int start_mysql_cleaner_ret = start_mysql_cleaner(&mysql_cleaner_config);
        if (start_mysql_cleaner_ret != 0) {
            has_error = 1;
        }
        */
    }

    // exit if any errors
    if (has_error) {
        fire_sigint_handler();
        return -1;
    }

    // loop
    while (1) {
        sleep(1);
    }
    return 0;
}
