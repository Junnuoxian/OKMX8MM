#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <stdint.h>

int64_t get_time_millis() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int64_t milliseconds = ((int64_t) tv.tv_sec) * 1000LL + tv.tv_usec / 1000L;
    return milliseconds;
}

int read_int(char *buffer, int start) {
    char byte3 = buffer[start + 0];
    char byte2 = buffer[start + 1];
    char byte1 = buffer[start + 2];
    char byte0 = buffer[start + 3];
    int value = ((byte3 & 0xFF) << 24) | ((byte2 & 0xFF) << 16) | ((byte1 & 0xFF) << 8) | (byte0 & 0xFF);
    return value;
}

int read_int_lsb(char *buffer, int start) {
    char byte0 = buffer[start + 0];
    char byte1 = buffer[start + 1];
    char byte2 = buffer[start + 2];
    char byte3 = buffer[start + 3];
    int value = ((byte3 & 0xFF) << 24) | ((byte2 & 0xFF) << 16) | ((byte1 & 0xFF) << 8) | (byte0 & 0xFF);
    return value;
}

int read_short(char *buffer, int start) {
    int h = buffer[start];
    int l = buffer[start + 1];
    int value = l | (h << 8);
    return value;
}

int read_short_lsb(char *buffer, int start) {
    int l = buffer[start];
    int h = buffer[start + 1];
    int value = l | (h << 8);
    return value;
}

float read_float(char *buffer, int start) {
    char value_buffer[4];
    value_buffer[3] = buffer[start + 0];
    value_buffer[2] = buffer[start + 1];
    value_buffer[1] = buffer[start + 2];
    value_buffer[0] = buffer[start + 3];
    float value = *((float *)value_buffer);
    return value;
}

int write_int(char *buffer, int start, int value) {
    buffer[start + 0] = (value >> 24) & 0xFF;
    buffer[start + 1] = (value >> 16) & 0xFF;
    buffer[start + 2] = (value >> 8) & 0xFF;
    buffer[start + 3] = (value >> 0) & 0xFF;
    return 4;
}

int write_float(char *buffer, int start, float value) {
    char value_buffer[4];
    float *float_p = (float *)value_buffer;
    *float_p = value;

    buffer[start + 0] = value_buffer[3];
    buffer[start + 1] = value_buffer[2];
    buffer[start + 2] = value_buffer[1];
    buffer[start + 3] = value_buffer[0];

    return 4;
}

void sprintf_bytes(char *str_buffer, char *buffer, int len) {
    for (int i = 0; i < len; i++) {
        sprintf(str_buffer + (i * 3), "%02X ", buffer[i]);
    }
}
