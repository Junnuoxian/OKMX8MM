#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/wait.h>

#include "mocker.h"
#include "log/log.h"

static pthread_t modbus_rtu_mocker_pthread;

// function declaration
void *do_start_modbus_rtu_mocker(void *modbus_rtu_void);

// implementation
int start_modbus_rtu_mocker(modbus_rtu_mock_t *modbus_rtu) {
    m_log(M_LOG_INFO, "Starting pthread for modbus rtu mocker...");
    int pthread_create_ret = pthread_create(
        &modbus_rtu_mocker_pthread,
        NULL,
        do_start_modbus_rtu_mocker,
        modbus_rtu);

    if (pthread_create_ret != 0) {
        m_log(M_LOG_ERROR, "Failed to create pthread for modbus rtu mocker.");
        return -1;
    }
    return 0;
}

void *do_start_modbus_rtu_mocker(void *modbus_rtu_void) {
    modbus_rtu_mock_t *modbus_rtu = (modbus_rtu_mock_t *)modbus_rtu_void;

    // set pthread name
    pthread_setname_np(pthread_self(), "MODBUS_RTU_MK");

    // open the uart file
    m_log(M_LOG_INFO, "Opening UART '%s'", modbus_rtu->uart_name);
    int uart_fd = open(modbus_rtu->uart_name, O_RDWR | O_NOCTTY | O_NDELAY);
    if (uart_fd < 0) {
        m_log(M_LOG_ERROR, "Failed to open UART '%s'", modbus_rtu->uart_name);
        return (void *) -1;
    }
    m_log(M_LOG_INFO, "Open UART '%s' successfully.", modbus_rtu->uart_name);

    // init uart
    m_log(M_LOG_INFO, "Initialize UART '%s'", modbus_rtu->uart_name);
    int init_uart_ret = init_uart(uart_fd, modbus_rtu);
    if (init_uart_ret != 0) {
        m_log(M_LOG_ERROR, "Failed to init UART '%s'", modbus_rtu->uart_name);
        return (void *) -1;
    }
    m_log(M_LOG_INFO, "Initialize UART '%s' successfully", modbus_rtu->uart_name);

    // device 1 regs
    float d1_regs[6];
    memset(d1_regs, 0, sizeof(d1_regs));

    // device 2 regs
    float d2_regs[4];
    memset(d2_regs, 0, sizeof(d2_regs));

    // error code
    int error_code=0x0000;

    // receive
    fd_set file_descriptor_set;
    FD_ZERO(&file_descriptor_set);
    FD_SET(uart_fd, &file_descriptor_set);
    fd_set saved_file_descriptor_set = file_descriptor_set;

    const int read_data_buffer_length = 256;
    char read_data_buffer[read_data_buffer_length];
    int reg_start = 0;
    int reg_count = 0;

    while (1) {
        int has_error = 0;
        int is_timeout = 0;
        int read_total = 0;
        while (1) {
            struct timeval timeout;
            timeout.tv_sec = 5;
            timeout.tv_usec = 0;

            // read the response, use select
            file_descriptor_set = saved_file_descriptor_set;
            FD_SET(uart_fd + 1, &file_descriptor_set);

            // select read events
            int select_ret = select(uart_fd + 1, &file_descriptor_set, NULL, NULL, &timeout);
            if (select_ret < 0) {
                // error
                m_log(M_LOG_ERROR, "Error occurs while select read events of '%s'", modbus_rtu->uart_name);
                has_error = 1;
                break;
            } else if (select_ret == 0) {
                // timeout
                m_log(M_LOG_ERROR, "Timeout while select read events of '%s'.", modbus_rtu->uart_name);
                is_timeout = 1;
                break;
            } else {
                if (FD_ISSET(uart_fd, &file_descriptor_set)) {
                    // read event
                    int available_to_read = 0;
                    int get_available_to_read_ret = ioctl(uart_fd, FIONREAD, &available_to_read);
                    if (get_available_to_read_ret < 0) {
                        m_log(M_LOG_ERROR, "Failed to get read available of '%s'", modbus_rtu->uart_name);
                        has_error = 1;
                        break;
                    }

                    int buffer_free_length = read_data_buffer_length - read_total;
                    int to_read = available_to_read > buffer_free_length ? buffer_free_length: available_to_read;
                    if (available_to_read > buffer_free_length) {
                        m_log(M_LOG_WARN, "Frame too large and exceed buffer, buffer capacity='%d', actual='%d'", read_data_buffer_length, read_total + to_read);
                        if (to_read <= 0) {
                            has_error = 1;
                            break;
                        }
                    }

                    int read_bytes = read(uart_fd, read_data_buffer + read_total, to_read);
                    if (read_bytes >= 0) {
                        read_total += read_bytes;
                    }

                    if (read_total >= 2) {
                        // check function code
                        int func_code = read_data_buffer[1] & 0xFF;
                        if (func_code != 0x04) {
                            m_log(M_LOG_ERROR, "Function code mismatched, expected '%d', actual '%d'", 0x04, func_code);
                            has_error = 1;
                            break;
                        }
                    }

                    if (read_total >= 4) {
                        // check data length
                        reg_start = read_short(read_data_buffer, 2);
                    }

                    if (read_total >= 6) {
                        reg_count = read_short(read_data_buffer, 4);
                    }

                    if (read_total >= 8) {
                        break;  // read done
                    }
                }
            }
        }

        if (is_timeout) {
            continue;
        }

        // response
        if (has_error == 0) {
            int device_address = read_data_buffer[0];
            int function_code = read_data_buffer[1];

            char frame_message_buffer[2048];
            sprintf_bytes(frame_message_buffer, read_data_buffer, read_total);
            m_log(M_LOG_INFO, "Receive packet: %s", frame_message_buffer);
            // m_log(M_LOG_INFO, "Reading registers from %d to %d", reg_start, reg_start + reg_count);

            if (0x01 <= device_address && device_address <= 0x04 || (0x04 <= device_address && device_address <= 0x07)) {
                char write_data_buffer[256];
                int wi = 0;
                write_data_buffer[wi++] = device_address;
                write_data_buffer[wi++] = function_code;
                write_data_buffer[wi++] = reg_count * 2;

                for (int i = reg_start; i < reg_start + reg_count/2; i++) {
                    d1_regs[i] = 100.0F + (rand() % 1000) / 1000.0F;
                    wi += write_float(write_data_buffer, wi, d1_regs[i]);
                }

                int crc16 = crc16_digest_v2(write_data_buffer, wi);
                write_data_buffer[wi++] = crc16 & 0xFF;
                write_data_buffer[wi++] = (crc16 >> 8) & 0xFF;

                char sending_message_buffer[2048];
                sprintf_bytes(sending_message_buffer, write_data_buffer, wi);
                m_log(M_LOG_INFO, "Sending packet: %s", sending_message_buffer);
                m_log(M_LOG_INFO, "Response CRC16 = %04X", crc16);
                write(uart_fd, write_data_buffer, wi);

            } else if (device_address == 0x02 || device_address==0x0A) {
                char write_data_buffer[256];
                int wi = 0;
                write_data_buffer[wi++] = device_address;
                write_data_buffer[wi++] = function_code;
                write_data_buffer[wi++] = reg_count * 2;

                for (int i = reg_start; i < reg_start + reg_count/2; i++) {
                    d2_regs[i] = 50.0F + (rand() % 1000) / 1000.0F;
                    if (i == 4) {
                        wi += write_int(write_data_buffer, wi, error_code);
                    } else {
                        wi += write_float(write_data_buffer, wi, d2_regs[i]);
                    }
                }

                int crc16 = crc16_digest_v2(write_data_buffer, wi);
                write_data_buffer[wi++] = crc16 & 0xFF;
                write_data_buffer[wi++] = (crc16 >> 8) & 0xFF;

                char sending_message_buffer[2048];
                sprintf_bytes(sending_message_buffer, write_data_buffer, wi);
                m_log(M_LOG_INFO, "Sending packet: %s", sending_message_buffer);
                m_log(M_LOG_INFO, "Response CRC16 = %04X", crc16);
                write(uart_fd, write_data_buffer, wi);
            }
        } else {
            char frame_message_buffer[2048];
            sprintf_bytes(frame_message_buffer, read_data_buffer, read_total);
            m_log(M_LOG_INFO, "Packet: %s", frame_message_buffer);

            m_log(M_LOG_ERROR, "Skip rest data for Modbus RTU bus");
            // skip data loop
            struct timeval skip_timeout;
            skip_timeout.tv_sec = 0;
            skip_timeout.tv_usec = 500 * 1000;  // 500ms

            while (1) {
                // read the data, use select
                file_descriptor_set = saved_file_descriptor_set;
                FD_SET(uart_fd + 1, &file_descriptor_set);

                // select read events
                int select_ret = select(uart_fd + 1, &file_descriptor_set, NULL, NULL, &skip_timeout);
                if (select_ret < 0) {
                    // error
                    m_log(M_LOG_ERROR, "Error occurs while select read events of '%s'", modbus_rtu->uart_name);
                    break;
                } else if (select_ret == 0) {
                    // timeout
                    m_log(M_LOG_ERROR, "Timeout while select select read events of '%s', will abandon this request.", modbus_rtu->uart_name);
                    break;
                } else {
                    if (FD_ISSET(uart_fd, &file_descriptor_set)) {
                        // read event
                        int available_to_read = 0;
                        int get_available_to_read_ret = ioctl(uart_fd, FIONREAD, &available_to_read);
                        if (get_available_to_read_ret < 0) {
                            m_log(M_LOG_ERROR, "Failed to get read available of '%s'", modbus_rtu->uart_name);
                            break;
                        }

                        const int dummy_buffer_length = 256;
                        char dummy_buffer[dummy_buffer_length];
                        int to_read = available_to_read < dummy_buffer_length ? available_to_read : dummy_buffer_length;
                        read(uart_fd, dummy_buffer, to_read);
                    }
                }
            }
        }
    }

    return (void *) 0;
}

int init_uart(int uart_fd, modbus_rtu_t *modbus_rtu) {
    struct termios uart_termios;
    int tcgetattr_ret = tcgetattr(uart_fd, &uart_termios);
    if (tcgetattr_ret != 0) {
        m_log(M_LOG_ERROR, "Failed to get attrs of UART '%s'", modbus_rtu->uart_name);
        return -1;
    }

    cfsetispeed(&uart_termios, modbus_rtu->bps);
    cfsetospeed(&uart_termios, modbus_rtu->bps);

    if (modbus_rtu->stop_bits == 1) {
        uart_termios.c_cflag &= ~CSTOPB;
    } else if (modbus_rtu->stop_bits == 2) {
        uart_termios.c_cflag |= CSTOPB;
    } else {
        m_log(M_LOG_ERROR, "Unkonwn stop bits setting '%d'", modbus_rtu->stop_bits);
    }

    if (modbus_rtu->parity_bits != 0) {
        uart_termios.c_cflag |= PARENB;
    } else {
        uart_termios.c_cflag &= ~PARENB;
    }

    // uart_termios.c_cflag &= ~CRTSCTS;
    uart_termios.c_cflag |= CREAD | CLOCAL;
    uart_termios.c_iflag &= ~(IXON | IXOFF | IXANY);
    uart_termios.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    uart_termios.c_oflag &= ~OPOST;
    cfmakeraw(&uart_termios);

    int tcsetattr_ret = tcsetattr(uart_fd, TCSANOW, &uart_termios);
    if (tcsetattr_ret != 0) {
        m_log(M_LOG_ERROR, "Failed to set attrs for uart '%s'", modbus_rtu->uart_name);
        return -1;
    }

    return 0;
}

int get_termios_data_bits_flag(int data_bits) {
    switch (data_bits) {
        case 8:
            return CS8;
        case 7:
            return CS7;
        case 6:
            return CS6;
        case 5:
            return CS5;
        default:
            m_log(M_LOG_WARN, "Unsupported data bits '%d', default to 8 bits.", data_bits);
            return CS8;
    }
}
