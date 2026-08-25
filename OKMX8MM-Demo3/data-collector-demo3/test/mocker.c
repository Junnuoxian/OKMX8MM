#include <stdio.h>
#include <stdlib.h>
#include <termios.h>

#include "log/log.h"
#include "mocker.h"

int main(int argc, char **args) {
    // log configuration
    m_log_config_t m_log_config;
    m_log_config.log_level = M_LOG_INFO;
    m_log_config.log_file_name_prefix = "/var/log/crrc-data-mocker";
    m_log_config.rolling_count = 5;                    // 5 files maximum
    m_log_config.rolling_file_size = 1 * 1024 * 1024;  // 5MB per one maximum
    int m_log_init_ret = m_log_init(&m_log_config);
    if (m_log_init_ret < 0) {
        printf("Failed to initialize logging.\n");
        return -1;
    }

    // modbus-rtu configuration
    modbus_rtu_mock_t modbus_rtu;
    modbus_rtu.uart_name = "/dev/ttymxc1";  // UART3
    modbus_rtu.bps = B9600;
    modbus_rtu.start_bits = 1;
    modbus_rtu.data_bits = 1;
    modbus_rtu.stop_bits = 1;
    modbus_rtu.parity_bits = 0;

    // public packets collector mocker configuration
    public_udp_config_mock_t public_udp_config;
    public_udp_config.host = "192.168.100.100";
    public_udp_config.udp_port = 7000;
    public_udp_config.target_can_id = 0x01E;

    // start
    int start_ret = 0;

    start_ret = start_modbus_rtu_mocker(&modbus_rtu);
    if (start_ret < 0) {
        return -1;
    }

    start_ret = start_public_udp_mocker(&public_udp_config);
    if (start_ret < 0) {
        return -1;
    }

    // wait
    while (1) {
        sleep(1);
    }

    return 0;
}
