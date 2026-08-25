#ifndef __MODBUS_RTU_H

#define __MODBUS_RTU_H 1

#include <stdint.h>
#include "mysql-config/mysql-config.h"
#include "file-storage/file-storage.h"
#include "modbus-rtu.h"

typedef enum device_type_enum {
    DEVICE_TYPE_OIL_SENSOR = 1,
    DEVICE_TYPE_COLLECT_MODULE = 2,
} device_type_t;

typedef enum error_code_enum {
    OK = 0,
    OFFLINE = 1,
} error_code_t;

typedef struct modbus_rtu_device_struct {
    char *device_name;
    int device_address;
    int device_type;

    // collect interval (ms)
    int collect_interval;

    // alarm threshold settings
    void *threshold_alarm_settings;

} modbus_rtu_device_t;

typedef struct modbus_rtu_struct {
    char *uart_name;
    int bps;
    int start_bits;
    int data_bits;
    int stop_bits;
    int parity_bits;

    // publish interval (ms)
    int publish_interval;

    // device config
    modbus_rtu_device_t **devices;
    int device_count;

    // mysql config
    mysql_config_t *mysql_config;

    // file storage config (for oil sensor metrics only)
    file_storage_config_t *file_storage_configs;
    int file_storage_config_count;
    
} modbus_rtu_t;

typedef struct oil_metrics_struct {
    char *device_name;
    int device_address;

    float temperature;
    float water_activity;
    float ppm;
    float viscosity;
    float density;
    float dielectric_constant;

    int64_t time_millis;
} oil_metrics_t;

typedef struct oil_alarm_threshold_struct {
    float temperature;
    float water_activity;
    float ppm;
    float viscosity;
    float density;
    float dielectric_constant;
} oil_alarm_threshold_t;

typedef struct collect_module_metrics_struct {
    char *device_name;
    int device_address;

    float abrasion;
    float temperature;
    float pressure;
    float board_temperature;
    int error_code;

    int64_t time_millis;
} collect_module_metrics_t;

typedef struct collect_module_alarm_threshold_struct {
    float abrasion;
    float temperature;
    float pressure;
    float board_temperature;
} collect_module_alarm_threshold_t;

typedef struct modbus_rtu_device_context_struct {
    // device config
    modbus_rtu_device_t *device;

    // next schedule time
    int64_t next_schedule_time;

    // schedule index, will save metrics data into metrics_list[schedule_index]
    int schedule_index;
    int schedule_count;

    // metrics list
    void *metrics_list;
    int metrics_list_size;

    // average result
    void *metrics;

    // alarm code
    int alarm_code;
    int error_code;

} modbus_rtu_device_context_t;

typedef struct oil_metrics_storage_struct {
    char device_name[32];
    int device_address;

    float temperature;
    float water_activity;
    float ppm;
    float viscosity;
    float density;
    float dielectric_constant;

    // flags
    uint8_t flags[4];

    // public packet timestamp
    int64_t public_packet_time_millis;

    // speed & km
    float speed;
    int km_post;

    // train/carriage no, temperature and double-heading status
    int train_serial_no;
    int temperature_outside;
    int locomotive_double_heading_status;
    int carriage_no;
    char train_no[8];

    // time
    int64_t time_millis;
} oil_metrics_storage_t;

int start_modbus_collector(modbus_rtu_t *modbus_rtu);
int public_udp_context_lock();
int public_udp_context_unlock();
#endif