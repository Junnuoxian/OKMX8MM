#ifndef __MOCKER_H

#define __MOCKER_H

#include "modbus-rtu/modbus-rtu.h"
#include "public/public-udp-collector.h"

typedef struct modbus_rtu_mock_struct {
    char *uart_name;
    int bps;
    int start_bits;
    int data_bits;
    int stop_bits;
    int parity_bits;

} modbus_rtu_mock_t;

typedef struct public_udp_config_mock_struct {
    // target UDP address
    char *host;
    int udp_port;
    
    // can destination
    int target_can_id;
} public_udp_config_mock_t;

typedef struct public_version_receiver_mock_struct {
    char *udp_host;
    int udp_port;
} public_version_receiver_mock_t;

typedef struct carriage_can_config_mock_struct {
    char *can_name;
    int bps;

    struct can_filter *can_receive_filters;
    int can_receive_filters_count;

} carriage_can_config_mock_t;

unsigned short crc16_digest_v2(const unsigned char *buf, unsigned int len);
int start_modbus_rtu_mocker(modbus_rtu_mock_t *modbus_rtu);
int start_public_udp_mocker(public_udp_config_mock_t *public_udp_config);
int start_carriage_can_receiver(carriage_can_config_mock_t *carriage_can_config);
int start_public_version_receiver(public_version_receiver_mock_t *public_version_config);

int send_data_can(int target_id, char *buffer, int len);
int process_provider_version_packet(char *receive_buffer, int data_length);
int process_oil_metrics_packet(char *receive_buffer, int data_length);

short read_short(char *buffer, int start);
float read_float(char *buffer, int start);
int write_short(char *buffer, int start, short value);
int write_int(char *buffer, int start, int value);
int write_float(char *buffer, int start, float value);
void sprintf_bytes(char *str_buffer, char *buffer, int len);
int64_t get_time_millis();
unsigned short crc16_digest_v2(const unsigned char *buf, unsigned int len);
uint32_t sum_digest(uint8_t *buf, int len);

#endif
