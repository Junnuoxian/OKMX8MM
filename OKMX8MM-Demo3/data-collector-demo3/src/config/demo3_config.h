#ifndef DEMO3_CONFIG_H
#define DEMO3_CONFIG_H

#define DEMO3_CONFIG_TEXT_LENGTH 128

typedef struct {
    char log_file_prefix[DEMO3_CONFIG_TEXT_LENGTH];
    char source[DEMO3_CONFIG_TEXT_LENGTH];
    char serial_device[DEMO3_CONFIG_TEXT_LENGTH];
    int baudrate;
    int start_bits;
    int data_bits;
    int stop_bits;
    int parity_bits;
    int sample_period_ms;

    char local_storage_path[DEMO3_CONFIG_TEXT_LENGTH];
    char sd_storage_path[DEMO3_CONFIG_TEXT_LENGTH];
    char storage_file_name[DEMO3_CONFIG_TEXT_LENGTH];
    int storage_compress;

    int sensor_power_enabled;
    int led_enabled;
    int led_running_pin;
    int led_sensor_error_pin;
    int led_error_pin;

    int mysql_enabled;
    char mysql_host[DEMO3_CONFIG_TEXT_LENGTH];
    int mysql_port;
    char mysql_user[DEMO3_CONFIG_TEXT_LENGTH];
    char mysql_password[DEMO3_CONFIG_TEXT_LENGTH];
    char mysql_database[DEMO3_CONFIG_TEXT_LENGTH];

    int mqtt_enabled;
    char mqtt_broker[DEMO3_CONFIG_TEXT_LENGTH];
    int mqtt_port;
    char mqtt_topic[DEMO3_CONFIG_TEXT_LENGTH];
    char mqtt_client_id[DEMO3_CONFIG_TEXT_LENGTH];

    int status_enabled;
    char status_path[DEMO3_CONFIG_TEXT_LENGTH];

    int ota_enabled;
    char ota_package_path[DEMO3_CONFIG_TEXT_LENGTH];
    char ota_staging_path[DEMO3_CONFIG_TEXT_LENGTH];
    char ota_reboot_marker_path[DEMO3_CONFIG_TEXT_LENGTH];

    char rpmsg_device[DEMO3_CONFIG_TEXT_LENGTH];
    int rpmsg_poll_timeout_ms;

    int can_enabled;
    char can_interface[DEMO3_CONFIG_TEXT_LENGTH];
    int can_id_base;

    char public_udp_host[DEMO3_CONFIG_TEXT_LENGTH];
    int public_udp_port;
} demo3_config_t;

void demo3_config_init(demo3_config_t *config);
int demo3_config_apply_line(demo3_config_t *config, const char *line);
int demo3_config_load(demo3_config_t *config, const char *path);

#endif
