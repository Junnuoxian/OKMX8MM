#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "mocker.h"
#include "log/log.h"

static pthread_t public_udp_mocker_pthread;
static pthread_t public_version_receiver_pthread;

// function declaration
void *do_start_public_udp_mocker(void *public_udp_config_void);

// implementation
int start_public_udp_mocker(public_udp_config_mock_t *public_udp_config) {
    m_log(M_LOG_INFO, "Starting pthread for public udp mocker...");
    int public_udp_mocker_pthread_create_ret = pthread_create(
        &public_udp_mocker_pthread,
        NULL,
        do_start_public_udp_mocker,
        public_udp_config);

    if (public_udp_mocker_pthread_create_ret != 0) {
        m_log(M_LOG_ERROR, "Failed to create pthread for public udp mocker.");
        return -1;
    }
}

void *do_start_public_udp_mocker(void *public_udp_config_void) {
    // cast
    public_udp_config_mock_t *public_udp_config = (public_udp_config_mock_t *)public_udp_config_void;

    // set pthread name
    pthread_setname_np(pthread_self(), "PUBLIC_INFO_MK");

    // initalize udp socket
    int udp_socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket_fd < 0) {
        m_log(M_LOG_ERROR, "Failed to open UDP socket");
        return (void *) -1;
    }

    // sock addr
    struct sockaddr_in sockaddr;
    memset(&sockaddr, 0, sizeof(struct sockaddr_in));
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_addr.s_addr = inet_addr(public_udp_config->host);
    sockaddr.sin_port = htons(public_udp_config->udp_port);

    // start sending
    unsigned char send_buffer[2048];
    int64_t next_send_sync_time = 0L;
    int64_t next_send_public_info_time = 0L;

    int64_t send_public_info_interval = 1000 * 1;
    int64_t send_sync_time_interval = 1000 * 10;

    int packet_seq = 0;
    int km_post = 0.0F;

    while (1) {
        // check the time, note that we should also check whether 
        // the next schedule time too long from now since the time synchronization
        int64_t time_now_millis = get_time_millis();
        if (time_now_millis >= next_send_public_info_time || next_send_public_info_time - time_now_millis > send_public_info_interval * 5) {
            next_send_public_info_time = time_now_millis + send_public_info_interval;
            m_log(M_LOG_INFO, "Sending public information");
            memset(send_buffer, 0, sizeof(send_buffer));

            int si = 0;
            send_buffer[si++] = 0xAA;
            send_buffer[si++] = 0xAA;

            // data length
            si++;
            si++;

            send_buffer[si++] = 0x01;
            send_buffer[si++] = 0xFF;
            send_buffer[si++] = packet_seq & 0xFF;
            send_buffer[si++] = ((packet_seq++) >> 8) & 0xFF;

            send_buffer[si++] = 0x05;
            send_buffer[si++] = 0x00;
            send_buffer[si++] = 0x00;
            send_buffer[si++] = 0x00;

            int64_t time_now_millis = get_time_millis();
            time_t time_now_seconds = time(NULL);
            struct tm *time_now = localtime(&time_now_seconds);

            int year = time_now->tm_year + 1900;
            int month = time_now->tm_mon + 1;
            int date = time_now->tm_mday;
            int hour = time_now->tm_hour;
            int min = time_now->tm_min;
            int sec = time_now->tm_sec;

            send_buffer[si++] = year - 2000;
            send_buffer[si++] = month;
            send_buffer[si++] = date;
            send_buffer[si++] = hour;
            send_buffer[si++] = min;
            send_buffer[si++] = sec;
            send_buffer[si++] = (time_now_millis % 1000) / 25;

            unsigned short speed = 300 * 100 + random() % 50;
            send_buffer[si++] = speed & 0xFF;
            send_buffer[si++] = (speed >> 8) & 0xFF;

            km_post += 1;
            si += write_int_lsb(send_buffer, si, km_post);

            // train no
            send_buffer[si++] = (2001) & 0xFF;
            send_buffer[si++] = (2001 >> 8) & 0xFF;

            send_buffer[si++] = -12; // temperature outside
            send_buffer[si++] = 1; // double-heading status
            send_buffer[si++] = 8; // carriage no

            // train no
            send_buffer[si++] = 'G';
            send_buffer[si++] = 0;
            send_buffer[si++] = '2';
            send_buffer[si++] = '5';
            send_buffer[si++] = '1';
            send_buffer[si++] = '2';
            send_buffer[si++] = 0;
            send_buffer[si++] = 0;

            // reserve
            si += 4;

            // data length
            int data_length = si + 2;
            send_buffer[2] = data_length & 0xFF;
            send_buffer[3] = (data_length >> 8) & 0xFF;

            // crc
            int crc16 = sum_digest(send_buffer, si);
            send_buffer[si++] = crc16 & 0xFF;
            send_buffer[si++] = (crc16 >> 8) & 0xFF;
            int send_length = si;

            // send via UDP
            m_log(M_LOG_INFO, "Sending %d bytes", send_length);
            int sent = sendto(udp_socket_fd, send_buffer, send_length, 0, (struct sockaddr *) &sockaddr, sizeof(sockaddr));
            if (sent < 0) {
                m_log(M_LOG_ERROR, "Failed to send udp data for public information");
            } else if (sent < send_length) {
                m_log(M_LOG_ERROR, "Unexpected sent data length for public version, expected: %d, actual: %d", send_length, sent);
            }

        } else if (time_now_millis >= next_send_sync_time || next_send_sync_time - time_now_millis > send_sync_time_interval * 5) {
            next_send_sync_time = time_now_millis + send_sync_time_interval;
            m_log(M_LOG_INFO, "Sending sync time packet");

            int si = 0;
            send_buffer[si++] = 0xAA;
            send_buffer[si++] = 0xAA;

            // data length
            si++;
            si++;

            send_buffer[si++] = 0x03;
            send_buffer[si++] = (packet_seq++) % 0xFF;
            send_buffer[si++] = 0X00;

            time_t time_now_seconds = time(NULL);
            struct tm *time_now = localtime(&time_now_seconds);

            int year = time_now->tm_year + 1900;
            int month = time_now->tm_mon + 1;
            int date = time_now->tm_mday;
            int hour = time_now->tm_hour;
            int min = time_now->tm_min;
            int sec = time_now->tm_sec;
            int millisec = (time_now_millis % 1000) / 10;

            send_buffer[si++] = year - 2000;
            send_buffer[si++] = month;
            send_buffer[si++] = date;
            send_buffer[si++] = hour;
            send_buffer[si++] = min;
            send_buffer[si++] = sec;
            send_buffer[si++] = millisec;

            si += 13;
            int data_length = si + 2;
            send_buffer[si++] = data_length & 0xFF;
            send_buffer[si++] = (data_length >> 8) & 0xFF;

            int crc16 = sum_digest(send_buffer, si);
            send_buffer[si++] = crc16 & 0xFF;
            send_buffer[si++] = (crc16 >> 8) & 0xFF;

            int send_length = si;

            // send via UDP
            int sent = sendto(udp_socket_fd, send_buffer, send_length, 0, (struct sockaddr *) &sockaddr, sizeof(sockaddr));
            if (sent < 0) {
                m_log(M_LOG_ERROR, "Failed to send udp data for sync time");
            } else if (sent < send_length) {
                m_log(M_LOG_ERROR, "Unexpected sent data length for sync time, expected: %d, actual: %d", send_length, sent);
            }
            m_log(M_LOG_INFO, "Sent sync packet time complete");
        }
        usleep(1000 * 100); // check every 100ms
    }

    // close socket
    close(udp_socket_fd);

    // exist pthread
    pthread_exit(0);
    return (void *) 0;
}
