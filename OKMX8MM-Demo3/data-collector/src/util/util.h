#ifndef __UTIL_H

#define __UTIL_H

int64_t get_time_millis();

float read_float(char *buffer, int start);
int read_short(char *buffer, int start);
int read_short_lsb(char *buffer, int start);
int read_int(char *buffer, int start);
int read_int_lsb(char *buffer, int start);

void sprintf_bytes(char *str_buffer, char *buffer, int len);

#endif