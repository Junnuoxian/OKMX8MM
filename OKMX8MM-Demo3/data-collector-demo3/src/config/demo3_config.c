#include "config/demo3_config.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *trim(char *text)
{
    char *end;

    while (*text != '\0' && isspace((unsigned char)*text)) {
        ++text;
    }
    end = text + strlen(text);
    while (end > text && isspace((unsigned char)end[-1])) {
        --end;
        *end = '\0';
    }
    return text;
}

static int copy_value(char *target, const char *value)
{
    size_t length = strlen(value);
    if (length >= DEMO3_CONFIG_TEXT_LENGTH) {
        return -1;
    }
    memcpy(target, value, length + 1u);
    return 0;
}

static int parse_int(const char *value, int *result)
{
    char *end;
    long parsed;

    errno = 0;
    parsed = strtol(value, &end, 10);
    if (value == end || errno == ERANGE || *trim(end) != '\0') {
        return -1;
    }
    if (parsed < -2147483647L - 1L || parsed > 2147483647L) {
        return -1;
    }
    *result = (int)parsed;
    return 0;
}

static int parse_bool(const char *value, int *result)
{
    if (strcmp(value, "1") == 0 || strcmp(value, "true") == 0 || strcmp(value, "yes") == 0) {
        *result = 1;
        return 0;
    }
    if (strcmp(value, "0") == 0 || strcmp(value, "false") == 0 || strcmp(value, "no") == 0) {
        *result = 0;
        return 0;
    }
    return -1;
}

#define APPLY_STRING(KEY, FIELD) \
    if (strcmp(key, KEY) == 0) return copy_value(config->FIELD, value)
#define APPLY_INT(KEY, FIELD) \
    if (strcmp(key, KEY) == 0) return parse_int(value, &config->FIELD)
#define APPLY_BOOL(KEY, FIELD) \
    if (strcmp(key, KEY) == 0) return parse_bool(value, &config->FIELD)

void demo3_config_init(demo3_config_t *config)
{
    memset(config, 0, sizeof(*config));
    (void)copy_value(config->log_file_prefix, "./runtime-data/data-collector");
    (void)copy_value(config->local_storage_path, "./runtime-data/data");
    (void)copy_value(config->sd_storage_path, "./runtime-data/sd");
    (void)copy_value(config->storage_file_name, "sample");
    (void)copy_value(config->mysql_host, "127.0.0.1");
    (void)copy_value(config->mysql_user, "root");
    (void)copy_value(config->mysql_database, "crrc-data-collector");
    (void)copy_value(config->mqtt_topic, "mine/demo3/sample");
    (void)copy_value(config->public_udp_host, "0.0.0.0");
    config->baudrate = 9600;
    config->start_bits = 1;
    config->data_bits = 8;
    config->stop_bits = 1;
    config->sample_period_ms = 1000;
    config->mysql_port = 3306;
    config->mqtt_port = 1883;
    config->public_udp_port = 7000;
}

int demo3_config_apply_line(demo3_config_t *config, const char *line)
{
    char buffer[DEMO3_CONFIG_TEXT_LENGTH * 2u];
    char *separator;
    char *key;
    char *value;

    if (config == 0 || line == 0 || strlen(line) >= sizeof(buffer)) {
        return -1;
    }
    (void)strcpy(buffer, line);
    key = trim(buffer);
    if (*key == '\0' || *key == '#') {
        return 0;
    }
    separator = strchr(key, '=');
    if (separator == 0) {
        return -1;
    }
    *separator = '\0';
    value = trim(separator + 1);
    key = trim(key);

    APPLY_STRING("log_file_prefix", log_file_prefix);
    APPLY_STRING("serial_device", serial_device);
    APPLY_INT("baudrate", baudrate);
    APPLY_INT("start_bits", start_bits);
    APPLY_INT("data_bits", data_bits);
    APPLY_INT("stop_bits", stop_bits);
    APPLY_INT("parity_bits", parity_bits);
    APPLY_INT("sample_period_ms", sample_period_ms);
    APPLY_STRING("local_storage_path", local_storage_path);
    APPLY_STRING("sd_storage_path", sd_storage_path);
    APPLY_STRING("storage_file_name", storage_file_name);
    APPLY_BOOL("storage_compress", storage_compress);
    APPLY_BOOL("sensor_power_enabled", sensor_power_enabled);
    APPLY_BOOL("led_enabled", led_enabled);
    APPLY_INT("led_running_pin", led_running_pin);
    APPLY_INT("led_sensor_error_pin", led_sensor_error_pin);
    APPLY_INT("led_error_pin", led_error_pin);
    APPLY_BOOL("mysql_enabled", mysql_enabled);
    APPLY_STRING("mysql_host", mysql_host);
    APPLY_INT("mysql_port", mysql_port);
    APPLY_STRING("mysql_user", mysql_user);
    APPLY_STRING("mysql_password", mysql_password);
    APPLY_STRING("mysql_database", mysql_database);
    APPLY_BOOL("mqtt_enabled", mqtt_enabled);
    APPLY_STRING("mqtt_broker", mqtt_broker);
    APPLY_INT("mqtt_port", mqtt_port);
    APPLY_STRING("mqtt_topic", mqtt_topic);
    APPLY_STRING("public_udp_host", public_udp_host);
    APPLY_INT("public_udp_port", public_udp_port);

    return 0;
}

int demo3_config_load(demo3_config_t *config, const char *path)
{
    FILE *file;
    char line[DEMO3_CONFIG_TEXT_LENGTH * 2u];
    int line_number = 0;

    if (config == 0 || path == 0) {
        return -1;
    }
    file = fopen(path, "r");
    if (file == 0) {
        return -2;
    }
    while (fgets(line, sizeof(line), file) != 0) {
        ++line_number;
        if (demo3_config_apply_line(config, line) != 0) {
            fclose(file);
            return -1000 - line_number;
        }
    }
    fclose(file);
    return 0;
}
