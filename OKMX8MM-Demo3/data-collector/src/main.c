#include <stdio.h>
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
#include "led/led.h"
#include "gpio/gpio.h"
#include "main.h"

int main(int argc, char **args) {
    printf("Starting ...\n");

    // signal handler
    init_signal_handler();

    // log configuration
    m_log_config_t m_log_config;
    m_log_config.log_level = M_LOG_INFO;
    m_log_config.log_file_name_prefix = "/var/log/crrc-data-collector";
    m_log_config.rolling_count = 5;                    // 5 files maximum
    m_log_config.rolling_file_size = 5 * 1024 * 1024;  // 5MB per one maximum
    int m_log_init_ret = m_log_init(&m_log_config);
    if (m_log_init_ret < 0) {
        printf("Failed to initialize logging.\n");
        return -1;
    }

    // log version information
    m_log(M_LOG_INFO, "%s (version: %s, build: %s %s, commit: %s)", ABOUT_NAME, ABOUT_VERSION, __DATE__, __TIME__, ABOUT_COMMIT);

    // set IP to 192.168.100.100/24
    m_log(M_LOG_INFO, "Setting IP address to 192.168.100.100 with netmask 255.255.255.0");
    int ifconfig_ret = system("ifconfig eth1 192.168.100.100 netmask 255.255.255.0");
    if (ifconfig_ret != 0) {
        m_log(M_LOG_ERROR, "Failed to set IP address to 192.168.100.100/24, ret=%d", ifconfig_ret);
        return -1;
    }

    // turn off 24V sensor power supply
    m_log(M_LOG_INFO, "Power off oil sensors ...");
    set_sensor_power_state(SENSOR_POWER_OFF);
    sleep(5);

    // 24v sensor power suplly on
    m_log(M_LOG_INFO, "Power on oil sensors ...");
    set_sensor_power_state(SENSOR_POWER_ON);
    sleep(4);

    // LED indicator controller
    led_config_t led_config;
    led_config.sys_running_led_gpio_pin = 123;
    led_config.sensor_comm_error_led_gpio_pin = 122;
    led_config.error_led_gpio_pin = 121;
    start_led_controller(&led_config);

    // init MySQL library
    int init_mysql_library_ret = init_mysql_library();
    if (init_mysql_library_ret != 0) {
        m_log(M_LOG_ERROR, "Failed to initialize MySQL library.");
        exit(-1);
    }

    // mysql configuration
    mysql_config_t mysql_config;
    mysql_config.conn_type = CONN_TYPE_SOCKET;
    mysql_config.host = "127.0.0.1";
    mysql_config.port = 3306;
    mysql_config.user = "root";
    mysql_config.passwd = "123456";
    mysql_config.database = "crrc-data-collector";

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
    modbus_rtu.uart_name = "/dev/ttymxc2";  // UART3
    modbus_rtu.bps = B9600;
    modbus_rtu.start_bits = 1;
    modbus_rtu.data_bits = 8;
    modbus_rtu.stop_bits = 1;
    modbus_rtu.parity_bits = 0;

    // publish interval
    modbus_rtu.publish_interval = 1000;  // 1s

    // file storage
    //板载
    const int FILE_STORAGE_CONFIG_COUNT = 2;
    file_storage_config_t file_storage_configs[FILE_STORAGE_CONFIG_COUNT];
    file_storage_configs[0].file_name = "oil-sensor";
    file_storage_configs[0].path = "/home/root/data";
    file_storage_configs[0].compress = 1;
    file_storage_configs[0].rolling_free_space =  500LL * 1024LL * 1024LL; // 500MB
    //SDcard
    file_storage_configs[1].file_name = "oil-sensor";
    file_storage_configs[1].path = "/media/sdcard/bin-data";
    file_storage_configs[1].compress = 0;
    file_storage_configs[1].rolling_free_space = 4LL * 1024LL * 1024LL * 1024LL; // 4GB

    modbus_rtu.file_storage_configs = file_storage_configs;
    modbus_rtu.file_storage_config_count = FILE_STORAGE_CONFIG_COUNT;
    
    // modbus-rtu devices
    const int MODBUS_RTU_DEVICE_COUNT = 4;
    const int modbus_rtu_device_count = MODBUS_RTU_DEVICE_COUNT;
    modbus_rtu_device_t *modbus_rtu_devices[MODBUS_RTU_DEVICE_COUNT];
    modbus_rtu.devices = modbus_rtu_devices;
    modbus_rtu.device_count = modbus_rtu_device_count;

    // delete the alarming rules
    oil_alarm_threshold_t oil_alarm_threshold;
    oil_alarm_threshold.temperature = 1000.0;
    oil_alarm_threshold.water_activity = 1000.0;
    oil_alarm_threshold.ppm = 1000.0;
    oil_alarm_threshold.viscosity = 1000.0;
    oil_alarm_threshold.density = 1000.0;
    oil_alarm_threshold.dielectric_constant = 1000.0;

    modbus_rtu_device_t modbus_rtu_oil_sensor_01;
    modbus_rtu_devices[0] = &modbus_rtu_oil_sensor_01;
    modbus_rtu_oil_sensor_01.device_type = DEVICE_TYPE_OIL_SENSOR;
    modbus_rtu_oil_sensor_01.device_name = "Oil Sensor 0x01";
    modbus_rtu_oil_sensor_01.device_address = 0x01;
    modbus_rtu_oil_sensor_01.collect_interval = 1000;  // 200ms, 5/s
    modbus_rtu_oil_sensor_01.threshold_alarm_settings = &oil_alarm_threshold;

    modbus_rtu_device_t modbus_rtu_oil_sensor_02;
    modbus_rtu_devices[1] = &modbus_rtu_oil_sensor_02;
    modbus_rtu_oil_sensor_02.device_type = DEVICE_TYPE_OIL_SENSOR;
    modbus_rtu_oil_sensor_02.device_name = "Oil Sensor 0x02";
    modbus_rtu_oil_sensor_02.device_address = 0x02;
    modbus_rtu_oil_sensor_02.collect_interval = 1000;  // 200ms, 5/s
    modbus_rtu_oil_sensor_02.threshold_alarm_settings = &oil_alarm_threshold;

    modbus_rtu_device_t modbus_rtu_oil_sensor_03;
    modbus_rtu_devices[2] = &modbus_rtu_oil_sensor_03;
    modbus_rtu_oil_sensor_03.device_type = DEVICE_TYPE_OIL_SENSOR;
    modbus_rtu_oil_sensor_03.device_name = "Oil Sensor 0x03";
    modbus_rtu_oil_sensor_03.device_address = 0x03;
    modbus_rtu_oil_sensor_03.collect_interval = 1000;  // 200ms, 5/s
    modbus_rtu_oil_sensor_03.threshold_alarm_settings = &oil_alarm_threshold;

    modbus_rtu_device_t modbus_rtu_oil_sensor_04;
    modbus_rtu_devices[3] = &modbus_rtu_oil_sensor_04;
    modbus_rtu_oil_sensor_04.device_type = DEVICE_TYPE_OIL_SENSOR;
    modbus_rtu_oil_sensor_04.device_name = "Oil Sensor 0x04";
    modbus_rtu_oil_sensor_04.device_address = 0x04;
    modbus_rtu_oil_sensor_04.collect_interval = 1000;  // 200ms, 5/s
    modbus_rtu_oil_sensor_04.threshold_alarm_settings = &oil_alarm_threshold;

    // public packets collector configuration
    public_udp_config_t public_udp_config;
    public_udp_config.mysql_config = &mysql_config;
    public_udp_config.udp_host = "0.0.0.0";
    public_udp_config.udp_port = 7000;

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

    // start modbus-rtu
    if (!has_error) {
        int start_modbus_collector_ret = start_modbus_collector(&modbus_rtu);
        if (start_modbus_collector_ret != 0) {
            has_error = 1;
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