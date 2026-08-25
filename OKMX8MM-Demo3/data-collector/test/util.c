#include <stdio.h>
#include <time.h>
#include <sys/time.h>

#include "mocker.h"

uint32_t sum_digest(uint8_t *buf, int len) {
    uint32_t sum = 0;
    for (int i=0; i<len; i++) {
        sum += buf[i] & 0xFF;
    }
    return sum;
}

unsigned short crc16_digest_v2(const unsigned char *buf, unsigned int len) {
    static const unsigned short table[2] = {0x0000, 0xA001};
    unsigned short crc = 0xFFFF;
    char bit = 0;
    unsigned int xor = 0;

    while (len--) {
        crc ^= (*buf++);

        for (bit = 0; bit < 8; bit++) {
            xor = crc & 1;
            crc >>= 1;
            crc ^= table[xor];
        }
    }

    return crc;
}

int64_t get_time_millis() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int64_t milliseconds = ((int64_t) tv.tv_sec) * 1000LL + tv.tv_usec / 1000L;
    return milliseconds;
}

short read_short(char *buffer, int start) {
    int h = buffer[start];
    int l = buffer[start + 1];
    int value = l | (h << 8);
    return value & 0xFFFF;
}

float read_float(char *buffer, int start) {
    char value_buffer[4];
    for (int i = 0; i < 4; i++) {
        value_buffer[3 - i] = buffer[start + i];
    }
    float value = *((float *)value_buffer);
    return value;
}

int write_short(char *buffer, int start, short value) {
    for (int i = 0; i < 2; i++) {
        buffer[start + i] = (value >> ((1 - i) * 8)) & 0xFF;
    }
    return 2;
}

int write_int(char *buffer, int start, int value) {
    for (int i = 0; i < 4; i++) {
        buffer[start + i] = (value >> ((3 - i) * 8)) & 0xFF;
    }
    return 4;
}

int write_int_lsb(char *buffer, int start, int value) {
    for (int i = 0; i < 4; i++) {
        buffer[start + i] = (value >> (i * 8)) & 0xFF;
    }
    return 4;
}

int write_float(char *buffer, int start, float value) {
    char value_buffer[4];
    float *float_p = (float *)value_buffer;
    *float_p = value;

    for (int i = 0; i < 4; i++) {
        buffer[start + i] = value_buffer[3 - i];
    }
    return 4;
}

void sprintf_bytes(char *str_buffer, char *buffer, int len) {
    for (int i = 0; i < len; i++) {
        sprintf(str_buffer + (i * 3), "%02X ", buffer[i]);
    }
}
