#ifndef __PUBLIC_UDP_COLLECTOR_H

#define __PUBLIC_UDP_COLLECTOR_H 1

#include <stdint.h>
#include "mysql-config/mysql-config.h"

typedef struct public_udp_config_struct {
    // receive data with host and port
    char *udp_host;
    int udp_port;

    // mysql config
    mysql_config_t *mysql_config;

} public_udp_config_t;

typedef struct public_packet_struct {
    // packet seq no
    int packet_no;

    // flags
    uint8_t flags[4];
    
    // date & time
    int year;
    int month;
    int date;
    int hour;
    int min;
    int second;
    int millisecond;

    // speed & km
    float speed;
    int km_post;

    // train/carriage no, temperature and double-heading status
    int train_serial_no;
    int temperature_outside;
    int locomotive_double_heading_status;
    int carriage_no;
    char train_no[8];
} public_packet_t;


typedef struct public_udp_context_struct {
    // config
    public_udp_config_t *public_udp_config;

    // udp socket file descriptor
    int udp_socket_fd;

    // mysql context
    mysql_context_t *mysql_context;

    // public packet instance
    public_packet_t public_packet;

    // time synchronized flag
    volatile int time_synchronized;

} public_udp_context_t;

int start_public_udp_collector(public_udp_config_t *pub_udp_confg);
public_udp_context_t *get_public_packet_context();

#endif