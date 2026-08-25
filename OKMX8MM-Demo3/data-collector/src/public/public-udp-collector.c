#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "global/global.h"
#include "util/util.h"
#include "crc/crc-16.h"
#include "log/log.h"
#include "signal-handler/signal-handler.h"
#include "mysql-config/mysql-config.h"
#include "led/led.h"
#include "public-udp-collector.h"

#define PACKET_TYPE_PUBLIC_INFO 0xFF01

// static variables
static volatile int shutdown_flag = 0;

static public_udp_context_t *public_udp_context;
static pthread_t public_udp_collector_thread;
static pthread_mutex_t public_udp_context_mutex;

// function declaration
void *do_start_public_udp_collector(void *public_udp_config_void);
static void on_shutdown();
static uint32_t sum_digest(uint8_t *buf, int len);
static int is_invalid_time(int64_t time);

// implementation
int start_public_udp_collector(public_udp_config_t *public_udp_config) {
    // udp context
    public_udp_context = (public_udp_context_t *)malloc(sizeof(public_udp_context_t));
    if (public_udp_context == NULL) {
        m_log(M_LOG_ERROR, "Failed to allocate memory for public udp context.");
        return -1;
    }
    memset(public_udp_context, 0, sizeof(public_udp_context_t));
    public_udp_context->public_udp_config = public_udp_config;

    // mutex
    memset((void *) &public_udp_context_mutex, 0, sizeof(pthread_mutex_t));
    int mutex_init_ret = pthread_mutex_init(&public_udp_context_mutex, NULL);
    if (mutex_init_ret != 0) {
        m_log(M_LOG_ERROR, "Failed to initialize mutext lock for public udp context");
        return -1;
    }

    // start thread
    m_log(M_LOG_INFO, "Starting pthread for public information collector...");
    int pthread_create_ret = pthread_create(
        &public_udp_collector_thread,
        NULL,
        do_start_public_udp_collector,
        public_udp_config);

    if (pthread_create_ret != 0) {
        m_log(M_LOG_ERROR, "Failed to create pthread for public information collector.");
        return -1;
    }
    return 0;
}

void *do_start_public_udp_collector(void *public_udp_config_void) {
    // cast
    public_udp_config_t *public_udp_config = (public_udp_config_t *)public_udp_config_void;

    // set pthread name
    pthread_setname_np(pthread_self(), "PUBLIC_INFO");

    // add shutdown callback
    add_shutdown_callback(on_shutdown);

    // init context
    int init_public_udp_context_ret = init_public_udp_context(public_udp_context, public_udp_config);
    if (init_public_udp_context_ret < 0) {
        m_log(M_LOG_ERROR, "Failed to initialize public udp context");
        dispose_public_udp_context(public_udp_context);
        free((void *) public_udp_context);
        return (void *) 0;
    }

    // start receiving
    m_log(M_LOG_INFO, "Receiving UDP packets...");
    #define RECEIVE_BUFFER_SIZE 2048
    unsigned char receive_buffer[RECEIVE_BUFFER_SIZE];
    char packet_message_buffer[RECEIVE_BUFFER_SIZE * 3 + 2];
    while (shutdown_flag == 0) {
        if (shutdown_flag != 0) {
            break;
        }
        // receive
        m_log(M_LOG_INFO, "Receiving data from UDP socket '%d'", public_udp_config->udp_port);
        memset(receive_buffer, 0, RECEIVE_BUFFER_SIZE);

        struct sockaddr_in source_sockaddr;
        int source_sockaddr_len = sizeof(source_sockaddr);

        int received = recvfrom(public_udp_context->udp_socket_fd, receive_buffer, RECEIVE_BUFFER_SIZE, 0, (struct sockaddr *) &source_sockaddr, &source_sockaddr_len);
        if (received <0 && errno == EAGAIN) {
            m_log(M_LOG_INFO, "Timeout receiving data from UDP socket '%d'", public_udp_config->udp_port);
            continue;
        }
        if (received < 0) {
            m_log(M_LOG_ERROR, "Failed to receive data from UDP socket '%d'", public_udp_config->udp_port);
            continue;
        }

        if (received < 5) {
            sprintf_bytes(packet_message_buffer, receive_buffer, received);
            m_log(M_LOG_ERROR, "Packet is too small and will skip it: %s", packet_message_buffer);
            continue;
        }

        if (!(receive_buffer[0] == 0xAA && receive_buffer[1] == 0xAA)) {
            sprintf_bytes(packet_message_buffer, receive_buffer, received);
            m_log(M_LOG_ERROR, "Packet header not 0xAAAA and will skip it: %s", packet_message_buffer);
            continue;
        }

        int packet_type = read_short_lsb(receive_buffer, 4);
        if (packet_type == PACKET_TYPE_PUBLIC_INFO) {
            sprintf_bytes(packet_message_buffer, receive_buffer, received);
            m_log(M_LOG_INFO, "Received public information UDP packet: %s", packet_message_buffer);

            process_public_info_packet(public_udp_context->mysql_context, receive_buffer, received);
        }
    }

    // dispose context
    dispose_public_udp_context(public_udp_context);

    // free resource
    free((void *) public_udp_context);
    public_udp_context = NULL;

    // exit
    return (void *) 0;
}

static void on_shutdown() {
    m_log(M_LOG_INFO, "Joining thread of public information collector(UDP)...");
    shutdown_flag = 1;
    pthread_join(public_udp_collector_thread, NULL);
    m_log(M_LOG_INFO, "Thread of public information collector(UDP) exited.");
}

int init_public_udp_context(public_udp_context_t *public_udp_context, public_udp_config_t *public_udp_config) {
    // initalize udp socket
    int udp_socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket_fd < 0) {
        m_log(M_LOG_ERROR, "Failed to open UDP socket");
        return -1;
    }
    public_udp_context->udp_socket_fd = udp_socket_fd;

    // recv timeout
    struct timeval timeout;
    timeout.tv_sec = 3; // 3s
    timeout.tv_usec = 0; 
    setsockopt(udp_socket_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    // sock addr
    struct sockaddr_in sockaddr;
    memset(&sockaddr, 0, sizeof(struct sockaddr_in));
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_addr.s_addr = inet_addr(public_udp_config->udp_host);
    sockaddr.sin_port = htons(public_udp_config->udp_port);
    int bind_ret = bind(public_udp_context->udp_socket_fd, (struct sockaddr *) &sockaddr, sizeof(sockaddr));
    if (bind_ret != 0) {
        m_log(M_LOG_INFO, "Failed to bind sockaddr with %s:%d", public_udp_config->udp_host, public_udp_config->udp_port);
        return -1;
    }

    // return
    return 0;
}

int dispose_public_udp_context(public_udp_context_t *public_udp_context) {
    // dispose mysql context
    if (public_udp_context->mysql_context != NULL) {
        dispose_mysql_context(public_udp_context->mysql_context);
        free(public_udp_context->mysql_context);
        public_udp_context->mysql_context = NULL;
    }

    // close udp socket fd
    if (public_udp_context->udp_socket_fd != 0) {
        close(public_udp_context->udp_socket_fd);
        m_log(M_LOG_INFO, "Close UDP socket.");
    }
}

int process_public_info_packet(mysql_context_t *mysql_context, char *buffer, int len) {
    int i=0;
    // packet header
    i+=2;

    // data length
    int data_length = read_short_lsb(buffer, i); i+=2;
    if (data_length > len) {
        m_log(M_LOG_ERROR, "Incorrect packet length detected: %d", data_length);
        return -1;
    }

    // packet type
    i+=2;

    // package seq
    int packet_no = read_short_lsb(buffer, i);
    i+=2;

    // byte 8-11 flags
    uint8_t flags[4];
    for(int j=0; j<4; j++, i++) {
        flags[j] = buffer[i];
    }
    int ccu_valid_flag = flags[0] & 0x01;
    int time_valid_flag = flags[0] & (0x01 << 2);

    // time
    int year = 2000 + (buffer[i] & 0xFF); i++;
    int month = buffer[i] & 0xFF; i++;
    int date = buffer[i] & 0xFF; i++;
    int hour = buffer[i] & 0xFF; i++;
    int min = buffer[i] & 0xFF; i++;
    int second = buffer[i] & 0xFF; i++;
    int millisec = buffer[i] & 0xFF; i++;
    millisec = (millisec & 0xFF) * 4;

    char packet_time[128];
    memset(packet_time, 0, sizeof(packet_time));
    sprintf(packet_time, "%04d-%02d-%02d %02d:%02d:%02d", year, month, date, hour, min, second);

    // speed & km
    float speed = (read_short_lsb(buffer, i) * 1.0) / 100; i+=2;
    int km_post = read_int_lsb(buffer, i); i+=4;

    // train serial no
    int train_serial_no = read_short_lsb(buffer, i);
    i+=2;

    // temperature outside
    int temperature_outside = (int8_t) buffer[i];
    i++;

    // locomotive double heading status
    int locomotive_double_heading_status = buffer[i];
    i++;
    
    // couch no
    int carriage_no = buffer[i];
    i++;

    // train no
    char train_no[12];
    memset(train_no, 0, sizeof(train_no));
    for (int j = 0, k = 0; j < 8; j++, i++) {
        train_no[k] = buffer[i];
        if (buffer[i]) {
            k++;
        }
    }

    int checksum = buffer[data_length - 2] | (buffer[data_length -1] << 8);
    int calc_checksum = sum_digest(buffer, data_length - 2);
    if (checksum != calc_checksum) {
        char packet_message_buffer[4*1024];
        sprintf_bytes(packet_message_buffer, buffer, len);
        m_log(M_LOG_ERROR, "Checksum mismatch, expected: '%04x', actual '%04X', data: %s", calc_checksum, checksum, packet_message_buffer);
        return -1;
    }

    int64_t time_now_millis = get_time_millis();
    time_t time_seconds = time(NULL);
    struct tm time_struct = *localtime(&time_seconds);
    char create_time[64];
    memset(create_time, 0, sizeof(create_time));
    sprintf(create_time, "%04d-%02d-%02d %02d:%02d:%02d", year, month, date, hour, min, second);

    // print packet decoded content
    m_log(M_LOG_INFO, "packet_time=%s, train_serial_no=%d, train_no=%s", packet_time, train_serial_no, train_no);

    // lock context
    int lock_ret = public_udp_context_lock();
    if (lock_ret != 0) {
        return -1;
    }

    // update the global instance
    public_packet_t *public_packet = &public_udp_context->public_packet;
    public_packet->packet_no = packet_no;
    memcpy(public_packet->flags, flags, sizeof(flags));
    public_packet->date = date;
    public_packet->year = year;
    public_packet->month = month;
    public_packet->date = date;
    public_packet->hour = hour;
    public_packet->min = min;
    public_packet->second = second;
    public_packet->millisecond = millisec;
    public_packet->speed = speed;
    public_packet->speed = speed;
    public_packet->km_post = km_post;
    public_packet->train_serial_no = train_serial_no & 0xFFFF;
    public_packet->temperature_outside = temperature_outside;
    public_packet->locomotive_double_heading_status = locomotive_double_heading_status;
    public_packet->carriage_no = carriage_no;
    strcpy(public_packet->train_no, train_no);

    // sync time on boot
    if (!public_udp_context->time_synchronized && time_valid_flag) {
        struct tm time_struct = { 0 };
        time_struct.tm_year = year - 1900;
        time_struct.tm_mon = month - 1;
        time_struct.tm_mday = date;
        time_struct.tm_hour = hour;
        time_struct.tm_min = min;
        time_struct.tm_sec = second;
        time_struct.tm_isdst = 0;
        time_t time_seconds = mktime(&time_struct);
        int set_time_ret = set_time(time_seconds);
        if (set_time_ret != 0) {
            m_log(M_LOG_ERROR, "Failed to set time to %04d-%02d-%02d %02d:%02d:%02d", year, month, hour, min, second);
        } else {
            m_log(M_LOG_INFO, "Set time to %04d-%02d-%02d %02d:%02d:%02d successfully", year, month, hour, min, second);
            public_udp_context->time_synchronized = 1;
        }
    }

    // unlock
    public_udp_context_unlock();

    // set comm led status
    led_set_public_packet_status(COMM_ACTIVE);

    // return
    return 0;
}

int set_time(time_t time_seconds) {
    if (is_invalid_time(time_seconds)) {
        m_log(M_LOG_ERROR, "Invalid time value: %u", time_seconds);
        return -1;
    }

    struct timeval tv;
    tv.tv_sec = time_seconds;
    tv.tv_usec = 0;

    // set time
    char packet_time[64];
    memset(packet_time, 0, sizeof(packet_time));
    struct tm *time_tm = localtime(&time_seconds);
    sprintf(packet_time, "%04d-%02d-%02d %02d:%02d:%02d", time_tm->tm_year + 1900, time_tm->tm_mon + 1, time_tm->tm_mday, time_tm->tm_hour, time_tm->tm_min, time_tm->tm_sec);

    m_log(M_LOG_INFO, "Setting system time to %s", packet_time);
    int settimeofday_ret = settimeofday(&tv, NULL);
    if (settimeofday_ret != 0) {
        m_log(M_LOG_ERROR, "Failed to set system time to %s", packet_time);
        return -1;
    }

    // save to hardware
    int hwclock_ret = system("hwclock -u --systohc");
    if (hwclock_ret != 0) {
        m_log(M_LOG_ERROR, "Failed to save time to hardware clock.");
        return -1;
    }

    m_log(M_LOG_INFO, "Set system time to %s successfully", packet_time);
    return 0;
}

public_udp_context_t *get_public_packet_context() {
    return public_udp_context;
}

int public_udp_context_lock() {
    int mutex_lock_ret = pthread_mutex_lock(&public_udp_context_mutex);
    if (mutex_lock_ret != 0) {
        m_log(M_LOG_ERROR, "Failed to acquire lock of public udp context.");
        return -1;
    }
    return 0;
}

int public_udp_context_unlock() {
    int mutex_unlock_ret = pthread_mutex_unlock(&public_udp_context_mutex);
    if (mutex_unlock_ret != 0) {
        m_log(M_LOG_ERROR, "Failed to release lock of public udp context.");
        return -1;
    }
    return 0;
}

static uint32_t sum_digest(uint8_t *buf, int len) {
    uint32_t sum = 0;
    for (int i=0; i<len; i++) {
        sum += buf[i] & 0xFF;
    }
    return sum;
}

static int is_invalid_time(int64_t time) {
    struct tm min_tm = {0};
    int min_input_elements = sscanf("2024-01-01", "%d-%02d-%02d", &(min_tm.tm_year), &(min_tm.tm_mon), &(min_tm.tm_mday));
    if (min_input_elements == 3) {
        min_tm.tm_year -= 1900;
        min_tm.tm_mon -= 1;
        int64_t min_timestamp = mktime(&min_tm);
        int64_t max_timestamp = min_timestamp + 3600LL * 24LL * 365LL * 50LL;
        if (time < min_timestamp || time > max_timestamp) {
            return -1;
        }
        return 0;
    } else {
        return -1;
    }
}