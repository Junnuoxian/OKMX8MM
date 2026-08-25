#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <mysql.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/wait.h>

#include "util/util.h"
#include "log/log.h"
#include "crc/crc-16.h"
#include "mysql-executor/mysql-executor.h"
#include "signal-handler/signal-handler.h"
#include "public/public-udp-collector.h"
#include "led/led.h"
#include "modbus-rtu.h"

#define TIME_SYNC_WAITS_MAX 10

// types
typedef struct modbus_rtu_context_struct {
    modbus_rtu_t *modbus_rtu_config;

    // uart file descriptor
    int uart_fd;

    // next publish time
    int64_t next_publish_time;

    // mysql context
    mysql_context_t *mysql_context;

    // file storage contexts
    file_storage_context_t *file_storage_contexts;
    
    // time synchronization waiting
    int time_sync_waits;

} modbus_rtu_context_t;

// static variables
static volatile int shutdown_flag = 0;
static pthread_t modbus_rtu_collector_pthread;
static pthread_t file_storage_cleaner_pthread;

// function declaration
static void *do_start_modbus_collector(void *modbus_rtu_void);
static int init_modbus_rtu_context(modbus_rtu_context_t *modbus_rtu_context, modbus_rtu_t *modbus_rtu);
static int init_uart(int uart_fd, modbus_rtu_t *modbus_rtu);
static int get_termios_data_bits_flag(int data_bits);
static void on_shutdown();

static int start_file_storage_cleaner(modbus_rtu_context_t *modbus_rtu_context, modbus_rtu_t *modbus_rtu);
static void *do_start_file_storage_cleaner(void *modbus_rtu_context_void);

static int collect_modbus_rtu_data(modbus_rtu_context_t *modbus_rtu_context, modbus_rtu_t *modbus_rtu);

static int save_oil_sensor_metrics(
    modbus_rtu_context_t *modbus_rtu_context,
    oil_metrics_t *oil_metrics);

static int save_collect_module_metrics(
    modbus_rtu_context_t *modbus_rtu_context,
    collect_module_metrics_t *collect_module_metrics);

static int modbus_rtu_read_registers(
    modbus_rtu_context_t *modbus_rtu_context,
    modbus_rtu_t *modbus_rtu,
    modbus_rtu_device_t *device,

    int start_reg_address,
    int read_reg_count,
    int reg_data_length,

    char *data_buffer);
    
static int collect_modbus_rtu_data_oil_sensor(
    modbus_rtu_context_t *modbus_rtu_context,
    modbus_rtu_t *modbus_rtu,
    modbus_rtu_device_t *device,
    oil_metrics_t *oil_metrics);

static int collect_modbus_rtu_data_collect_module(
    modbus_rtu_context_t *modbus_rtu_context,
    modbus_rtu_t *modbus_rtu,
    modbus_rtu_device_t *device,
    collect_module_metrics_t *collect_module_metrics);

// implementation
int start_modbus_collector(modbus_rtu_t *modbus_rtu) {
    m_log(M_LOG_INFO, "Starting pthread for MODBUS RTU collector...");
    int pthread_create_ret = pthread_create(
        &modbus_rtu_collector_pthread,
        NULL,
        do_start_modbus_collector,
        modbus_rtu);

    if (pthread_create_ret != 0) {
        m_log(M_LOG_ERROR, "Failed to create pthread for MODBUS RTU collector.");
        return -1;
    }
    return 0;
}

static void *do_start_modbus_collector(void *modbus_rtu_void) {
    // cast
    modbus_rtu_t *modbus_rtu = (modbus_rtu_t *)modbus_rtu_void;

    // set pthread name
    pthread_setname_np(pthread_self(), "MODBUS_RTU");

    // init the context
    modbus_rtu_context_t modbus_rtu_context;
    memset(&modbus_rtu_context, 0, sizeof(modbus_rtu_context_t));

    // init uart and MySQL connection
    int init_modbus_rtu_context_ret = init_modbus_rtu_context(&modbus_rtu_context, modbus_rtu);
    if (init_modbus_rtu_context_ret != 0) {
        m_log(M_LOG_ERROR, "Failed to initialize MODBUS RTU context");
        exit(-1);
        return (void *) 0;
    }

    // start the file storage cleaner
    start_file_storage_cleaner(&modbus_rtu_context, modbus_rtu);

    // add shutdown hook
    add_shutdown_callback(on_shutdown);

    // start collecting data
    collect_modbus_rtu_data(&modbus_rtu_context, modbus_rtu);

    // close
    dispose_modbus_rtu_context(&modbus_rtu_context, modbus_rtu);

    // return
    return (void *) 0;
}

static void on_shutdown() {
    m_log(M_LOG_INFO, "Joining thread of MODBUS RTU collector...");
    shutdown_flag = 1;
    pthread_join(modbus_rtu_collector_pthread, NULL);
    pthread_join(file_storage_cleaner_pthread, NULL);
    m_log(M_LOG_INFO, "Thread of MODBUS RTU collector exited.");
}

static int init_modbus_rtu_context(modbus_rtu_context_t *modbus_rtu_context, modbus_rtu_t *modbus_rtu) {
    modbus_rtu_context->modbus_rtu_config = modbus_rtu;

    // open the uart file
    m_log(M_LOG_INFO, "Opening UART '%s'", modbus_rtu->uart_name);
    int uart_fd = open(modbus_rtu->uart_name, O_RDWR | O_NOCTTY | O_NDELAY);
    if (uart_fd < 0) {
        m_log(M_LOG_ERROR, "Failed to open UART '%s'", modbus_rtu->uart_name);
        return -1;
    }
    modbus_rtu_context->uart_fd = uart_fd;
    m_log(M_LOG_INFO, "Open UART '%s' successfully.", modbus_rtu->uart_name);

    // init uart
    m_log(M_LOG_INFO, "Initialize UART '%s'", modbus_rtu->uart_name);
    int init_uart_ret = init_uart(uart_fd, modbus_rtu);
    if (init_uart_ret != 0) {
        m_log(M_LOG_ERROR, "Failed to init UART '%s'", modbus_rtu->uart_name);
        return -1;
    }
    m_log(M_LOG_INFO, "Initialize UART '%s' successfully", modbus_rtu->uart_name);

    // init storage context
    m_log(M_LOG_INFO, "Initialize file storage contexts");
    file_storage_config_t *file_storage_configs = modbus_rtu->file_storage_configs;
    int file_storage_config_count = modbus_rtu->file_storage_config_count;
    file_storage_context_t *file_storage_contexts = (file_storage_context_t *) malloc(sizeof(file_storage_context_t) * file_storage_config_count);
    memset(file_storage_contexts, 0, sizeof(file_storage_context_t) * file_storage_config_count);
    for(int i=0; i<file_storage_config_count; i++) {
        mkdirs(file_storage_configs[i].path);
        file_storage_contexts[i].file_storage_config = file_storage_configs + i;
        file_storage_contexts[i].file_fd = -1;
        if (strstr(file_storage_contexts[i].file_storage_config->path, "/media/sdcard/") != NULL) {
            int sdcard_mounted = file_storage_check_sdcard_mount();
            if (sdcard_mounted != 0) {
                file_storage_contexts[i].disabled = 1;
                m_log(M_LOG_ERROR, "SD card not mounted, disable data writing for '%s'", file_storage_contexts[i].file_storage_config->path);
            }
        }
    }
    modbus_rtu_context->file_storage_contexts = file_storage_contexts;

    // return
    return 0;
}

int dispose_modbus_rtu_context(modbus_rtu_context_t *modbus_rtu_context, modbus_rtu_t *modbus_rtu) {
    
    if (modbus_rtu_context->uart_fd != 0) {
        tcflush(modbus_rtu_context->uart_fd, TCIFLUSH);
        close(modbus_rtu_context->uart_fd);
        modbus_rtu_context->uart_fd = 0;
        m_log(M_LOG_INFO, "Close uart '%s'", modbus_rtu->uart_name);
    }

    dispose_mysql_context(modbus_rtu_context->mysql_context);
    free(modbus_rtu_context->mysql_context);
    modbus_rtu_context->mysql_context = NULL;

    if (modbus_rtu_context->file_storage_contexts) {
        for(int i=0; i<modbus_rtu->file_storage_config_count; i++) {
            file_storage_context_t *file_storage_context = modbus_rtu_context->file_storage_contexts + i;
            file_storage_close(file_storage_context);
        }
        free(modbus_rtu_context->file_storage_contexts);
    }

    return 0;
}

static int start_file_storage_cleaner(modbus_rtu_context_t *modbus_rtu_context, modbus_rtu_t *modbus_rtu) {
    m_log(M_LOG_INFO, "Starting pthread for file storage cleaner...");
    int pthread_create_ret = pthread_create(
        &file_storage_cleaner_pthread,
        NULL,
        do_start_file_storage_cleaner,
        modbus_rtu_context);

    if (pthread_create_ret != 0) {
        m_log(M_LOG_ERROR, "Failed to create pthread for file storage cleaner.");
        return -1;
    }
    return 0;
}

static void *do_start_file_storage_cleaner(void *modbus_rtu_context_void) {
    modbus_rtu_context_t *modbus_rtu_context = (modbus_rtu_context_t *) modbus_rtu_context_void;
    // set thread name
    pthread_setname_np(pthread_self(), "FILE_STORAGE_ROLLING");

    // perform rolling every 1min
    const int rolling_interval = 60 * 1000;
    int64_t next_rolling_time = 0;

    const int sync_interval = 10 * 1000;
    int64_t next_sync_time = 0;

    // collect loop
    while (shutdown_flag == 0) {
        // check every 1s
        sleep(1);
        if (shutdown_flag != 0) {
            break;
        }

        // collect device metrics
        int64_t time_now_millis = get_time_millis();

        // check, note that we should also check whether 
        // the next schedule time too long from now since the time synchronization
        if (next_rolling_time <= time_now_millis
        || next_rolling_time - time_now_millis > rolling_interval) {
            next_rolling_time = time_now_millis + rolling_interval;
            // rolling
            for(int i=0; i<modbus_rtu_context->modbus_rtu_config->file_storage_config_count; i++) {
                file_storage_context_t *file_storage_context = modbus_rtu_context->file_storage_contexts + i;
                file_storage_rolling(file_storage_context);
                file_storage_compress(file_storage_context);
            }

            // check sdcard mount
            file_storage_check_sdcard_mount();
        }

        if (next_sync_time <= time_now_millis
        || next_sync_time - time_now_millis > sync_interval) {
            next_sync_time = time_now_millis + sync_interval;
            // sync
            for(int i=0; i<modbus_rtu_context->modbus_rtu_config->file_storage_config_count; i++) {
                file_storage_context_t *file_storage_context = modbus_rtu_context->file_storage_contexts + i;
                if (file_storage_context->file_fd >= 0) {
                    int fsync_ret = fsync(file_storage_context->file_fd);
                    if (fsync_ret != 0) {
                        m_log(M_LOG_ERROR, "Failed to call fsync to file %s", file_storage_context->file_path);
                    }
                }
            }
        }
    }

    // return
    return (void *) 0;
}

static int collect_modbus_rtu_data(modbus_rtu_context_t *modbus_rtu_context, modbus_rtu_t *modbus_rtu) {
    modbus_rtu_device_t **devices = modbus_rtu->devices;
    int device_count = modbus_rtu->device_count;

    // initialize the device contexts
    modbus_rtu_device_context_t *device_contexts = (modbus_rtu_device_context_t *)malloc(sizeof(modbus_rtu_device_context_t) * device_count);
    memset(device_contexts, 0, sizeof(modbus_rtu_device_context_t) * device_count);

    for (int i = 0; i < device_count; i++) {
        modbus_rtu_device_context_t *device_context = device_contexts + i;
        modbus_rtu_device_t *device = devices[i];
        device_context->device = device;

        // collect multi times during publish interval
        device_context->metrics_list_size = modbus_rtu->publish_interval / device->collect_interval + 2;  // +2 to avoid segment fault (illegal memory access), since we might collect 1/2 times more
        if (device->device_type == DEVICE_TYPE_OIL_SENSOR) {
            device_context->metrics_list = (oil_metrics_t *)malloc(sizeof(oil_metrics_t) * device_context->metrics_list_size);
            device_context->metrics = (oil_metrics_t *)malloc(sizeof(oil_metrics_t));
        } else if (device->device_type == DEVICE_TYPE_COLLECT_MODULE) {
            device_context->metrics_list = (collect_module_metrics_t *)malloc(sizeof(collect_module_metrics_t) * device_context->metrics_list_size);
            device_context->metrics = (oil_metrics_t *)malloc(sizeof(collect_module_metrics_t));
        } else {
            m_log(M_LOG_ERROR, "Unknown device type '%d' of device '%s'", device->device_type, device->device_name);
        }
    }

    int overall_alarm_code = 0;
    int overall_error_code = 0;

    // collect loop
    while (shutdown_flag == 0) {
        // check every 5ms
        usleep(5000); 
        if (shutdown_flag != 0) {
            break;
        }

        // collect device metrics
        int64_t time_now_millis = get_time_millis();
        for (int device_index = 0; device_index < device_count; device_index++) {
            modbus_rtu_device_context_t *device_context = device_contexts + device_index;

            // check the schedule time, note that we should also check whether 
            // the next schedule time too long from now since the time synchronization
            int schedule = 0;
            int64_t device_next_schedule_time = device_context->next_schedule_time;
            if (time_now_millis >= device_next_schedule_time || device_next_schedule_time - time_now_millis > device_context->device->collect_interval * 10) {
                schedule = 1;
            }
            if (!schedule) {
                continue;
            }

            modbus_rtu_device_t *device = devices[device_index];
            m_log(M_LOG_INFO, "Collect metrics data of '%s'", device->device_name);

            int collect_ret;
            int device_type = device->device_type;
            if (device_type == DEVICE_TYPE_OIL_SENSOR) {
                oil_metrics_t *oil_metrics_list = (oil_metrics_t *)device_context->metrics_list;
                oil_metrics_t *oil_metrics = oil_metrics_list + device_context->schedule_index;
                collect_ret = collect_modbus_rtu_data_oil_sensor(modbus_rtu_context, modbus_rtu, device, oil_metrics);
            } else if (device_type == DEVICE_TYPE_COLLECT_MODULE) {
                collect_module_metrics_t *collect_module_metrics_list = (collect_module_metrics_t *)device_context->metrics_list;
                collect_module_metrics_t *collect_module_metrics = collect_module_metrics_list + device_context->schedule_index;
                collect_ret = collect_modbus_rtu_data_collect_module(modbus_rtu_context, modbus_rtu, device, collect_module_metrics);
            }

            if (collect_ret == 0) {
                device_context->schedule_index++;
                device_context->schedule_count++;
                device_context->schedule_index %= device_context->metrics_list_size;
                device_context->schedule_count = device_context->schedule_count > device_context->metrics_list_size ? device_context->metrics_list_size : device_context->schedule_count;
                led_set_sensor_error_status(device_index, STATUS_NORMAL);
                usleep(100 * 1000);
            } else {
                overall_error_code = 1;
                led_set_sensor_error_status(device_index, STATUS_ERROR);
            }

            int64_t new_device_next_schedule_time = time_now_millis + device->collect_interval;
            device_context->next_schedule_time = new_device_next_schedule_time;
        }

        // publish, note that we should also check whether 
        // the next schedule time too long from now since the time synchronization
        if (modbus_rtu_context->next_publish_time <= time_now_millis
        || modbus_rtu_context->next_publish_time - time_now_millis > modbus_rtu->publish_interval * 10) {
            modbus_rtu_context->next_publish_time = time_now_millis + modbus_rtu->publish_interval;

            // average metrics
            for (int i = 0; i < device_count; i++) {
                modbus_rtu_device_t *device = devices[i];
                modbus_rtu_device_context_t *device_context = device_contexts + i;

                if (device->device_type == DEVICE_TYPE_OIL_SENSOR) {
                    oil_metrics_t oil_metrics;
                    memset(&oil_metrics, 0, sizeof(oil_metrics_t));
                    oil_metrics.device_name = device->device_name;
                    oil_metrics.device_address = device->device_address;
                    oil_metrics.time_millis = time_now_millis;

                    oil_metrics_t *oil_metrics_list = (oil_metrics_t *)device_context->metrics_list;
                    for (int j = 0; j < device_context->schedule_count; j++) {
                        oil_metrics.temperature += oil_metrics_list[j].temperature;
                        oil_metrics.water_activity += oil_metrics_list[j].water_activity;
                        oil_metrics.ppm += oil_metrics_list[j].ppm;
                        oil_metrics.viscosity += oil_metrics_list[j].viscosity;
                        oil_metrics.density += oil_metrics_list[j].density;
                        oil_metrics.dielectric_constant += oil_metrics_list[j].dielectric_constant;
                    }
                    if (device_context->schedule_count > 0) {
                        oil_metrics.temperature /= device_context->schedule_count;
                        oil_metrics.water_activity /= device_context->schedule_count;
                        oil_metrics.ppm /= device_context->schedule_count;
                        oil_metrics.viscosity /= device_context->schedule_count;
                        oil_metrics.density /= device_context->schedule_count;
                        oil_metrics.dielectric_constant /= device_context->schedule_count;
                        
                        // compare with threshold
                        int alarm_code = 0x00;
                        oil_alarm_threshold_t *oil_alarm_threshold = (oil_alarm_threshold_t *) device->threshold_alarm_settings;
                        if (oil_metrics.temperature >= oil_alarm_threshold->temperature) {
                            alarm_code |= 1<<0;
                        }
                        if (oil_metrics.water_activity >= oil_alarm_threshold->water_activity) {
                            alarm_code |= 1<<1;
                        }
                        if (oil_metrics.ppm >= oil_alarm_threshold->ppm) {
                            alarm_code |= 1<<2;
                        }
                        if (oil_metrics.viscosity >= oil_alarm_threshold->viscosity) {
                            alarm_code |= 1<<3;
                        }
                        if (oil_metrics.density >= oil_alarm_threshold->density) {
                            alarm_code |= 1<<4;
                        }
                        if (oil_metrics.dielectric_constant >= oil_alarm_threshold->dielectric_constant) {
                            alarm_code |= 1<<5;
                        }
                        device_context->alarm_code = alarm_code;

                        // save to context
                        *((oil_metrics_t *) device_context->metrics) = oil_metrics;

                        // save to MySQL
                        int save_ret = save_oil_sensor_metrics(modbus_rtu_context, &oil_metrics);
                        if (save_ret != 0) {
                            overall_error_code = 1;
                        }
                    } else {
                        // save to context
                        *((oil_metrics_t *) device_context->metrics) = oil_metrics;
                        device_context->error_code = 0x01;
                    }

                } else if (device->device_type == DEVICE_TYPE_COLLECT_MODULE) {
                    collect_module_metrics_t collect_module_metrics;
                    memset(&collect_module_metrics, 0, sizeof(collect_module_metrics_t));
                    collect_module_metrics.device_name = device->device_name;
                    collect_module_metrics.device_address = device->device_address;
                    collect_module_metrics.time_millis = time_now_millis;

                    int abrasion_count=0;
                    int temperature_count=0;
                    int pressure_count=0;
                    int board_temperature_count=0;
                    collect_module_metrics_t *collect_module_metrics_list = (collect_module_metrics_t *)device_context->metrics_list;
                    for (int j = 0; j < device_context->schedule_count; j++) {
                        int error_code=collect_module_metrics_list[j].error_code;
                        collect_module_metrics.error_code |= error_code;
                        if((error_code & 0X000F)==0) {
                            collect_module_metrics.abrasion += collect_module_metrics_list[j].abrasion;
                            abrasion_count++;
                        }
                        if((error_code & 0x00F0)==0) {
                            collect_module_metrics.temperature += collect_module_metrics_list[j].temperature;
                            temperature_count++;
                        }
                        if((error_code & 0x0F00)==0) {
                            collect_module_metrics.pressure += collect_module_metrics_list[j].pressure;
                            pressure_count++;
                        }
                        if((error_code & 0xF000)==0) {
                            collect_module_metrics.board_temperature += collect_module_metrics_list[j].board_temperature;
                            board_temperature_count++;
                        }
                    }
                    if (device_context->schedule_count > 0) {
                        if(abrasion_count>0) {
                            collect_module_metrics.abrasion /= abrasion_count;
                            collect_module_metrics.error_code &= 0xFFF0;
                        }
                        if(temperature_count>0) {
                            collect_module_metrics.temperature /= temperature_count;
                            collect_module_metrics.error_code &= 0xFF0F;
                        }
                        if(pressure_count>0) {
                            collect_module_metrics.pressure /= pressure_count;
                            collect_module_metrics.error_code &= 0xF0FF;
                        }
                        if(board_temperature_count>0) {
                            collect_module_metrics.board_temperature /= board_temperature_count;
                            collect_module_metrics.error_code &= 0x0FFF;
                        }

                        // set error code
                        device_context->error_code = collect_module_metrics.error_code;

                        // compare with threshold
                        int alarm_code = 0x00;
                        collect_module_alarm_threshold_t *collect_module_alarm_threshold = (collect_module_alarm_threshold_t *) device->threshold_alarm_settings;
                        if (collect_module_metrics.abrasion >= collect_module_alarm_threshold-> abrasion) {
                            alarm_code |= 1 << 0;
                        }
                        if (collect_module_metrics.temperature >= collect_module_alarm_threshold-> temperature) {
                            alarm_code |= 1 << 1;
                        }
                        if (collect_module_metrics.pressure >= collect_module_alarm_threshold-> pressure) {
                            alarm_code |= 1 << 2;
                        }
                        if (collect_module_metrics.board_temperature >= collect_module_alarm_threshold-> board_temperature) {
                            alarm_code |= 1 << 3;
                        }
                        device_context->alarm_code = alarm_code;

                        // save to context
                        *((collect_module_metrics_t *) device_context->metrics) = collect_module_metrics; 

                        // save to MySQL
                        int save_ret = save_collect_module_metrics(modbus_rtu_context, &collect_module_metrics);
                        if (save_ret != 0) {
                            overall_error_code = 1;
                        }
                    } else {
                        // save to context
                        *((collect_module_metrics_t *) device_context->metrics) = collect_module_metrics; 
                        collect_module_metrics.error_code = 0xEEEE;
                    }
                }
                
                // reset schedule index/count
                device_context->schedule_index = 0;
                device_context->schedule_count = 0;

                // overall
                if (device_context->error_code) {
                    overall_alarm_code = 1;
                }
                if (device_context->alarm_code) {
                    // overall_error_code = 1;
                }
            }

            // send via udp/can
            modbus_rtu_device_context_t **oil_device_contexts = (modbus_rtu_device_context_t **)malloc(sizeof(void *) * device_count);
            memset(oil_device_contexts, 0, sizeof(void *) * device_count);
            
            int oil_device_count = 0;
            for (int i = 0; i < device_count; i++) {
                modbus_rtu_device_t *device = devices[i];
                modbus_rtu_device_context_t *device_context = device_contexts + i;
                if (device->device_type == DEVICE_TYPE_OIL_SENSOR) {
                    oil_device_contexts[oil_device_count++] = device_context;
                }
            }

            // set modbus error status
            overall_alarm_code = 0;
            overall_error_code = 0;
        }

        // check shutdown flag before sleep
        if (shutdown_flag) {
            break;
        }
    }

    // free
    for (int i = 0; i < device_count; i++) {
        modbus_rtu_device_context_t *device_context = device_contexts + i;
        free(device_context->metrics_list);
        free(device_context->metrics);
    }
    free(device_contexts);
}

static int collect_modbus_rtu_data_oil_sensor(
    modbus_rtu_context_t *modbus_rtu_context,
    modbus_rtu_t *modbus_rtu,
    modbus_rtu_device_t *device,
    oil_metrics_t *oil_metrics) {
    // request data through MODBUS RTU protocol
    m_log(M_LOG_INFO, "Reading registers from oil sensor '%s' by MODBUS RTU protocol", device->device_name);
    char data[256];
    int modbus_rtu_request_ret = modbus_rtu_read_registers(
        modbus_rtu_context,
        modbus_rtu,
        device,
        0x00,
        6 * 2,
        6 * 4,
        data);
    if (modbus_rtu_request_ret < 0) {
        return -1;
    }

    float temperature = read_float(data, 0);
    float water_activity = read_float(data, 4);
    float ppm = read_float(data, 8);
    float viscosity = read_float(data, 12);
    float density = read_float(data, 16);
    float dielectric_constant = read_float(data, 20);

    oil_metrics->device_name = device->device_name;
    oil_metrics->device_address = device->device_address;

    oil_metrics->temperature = temperature;
    oil_metrics->water_activity = water_activity;
    oil_metrics->ppm = ppm;
    oil_metrics->viscosity = viscosity;
    oil_metrics->density = density;
    oil_metrics->dielectric_constant = dielectric_constant;

    return 0;
}

static int save_oil_sensor_metrics(
    modbus_rtu_context_t *modbus_rtu_context,
    oil_metrics_t *oil_metrics) {
    
    // build the storage instance
    oil_metrics_storage_t oil_metrics_storage = { 0 };
    strcpy(oil_metrics_storage.device_name, oil_metrics->device_name);
    oil_metrics_storage.device_address = oil_metrics->device_address;
    oil_metrics_storage.temperature = oil_metrics->temperature;
    oil_metrics_storage.water_activity = oil_metrics->water_activity;
    oil_metrics_storage.ppm = oil_metrics->ppm;
    oil_metrics_storage.viscosity = oil_metrics->viscosity;
    oil_metrics_storage.density = oil_metrics->density;
    oil_metrics_storage.dielectric_constant = oil_metrics->dielectric_constant;
    oil_metrics_storage.time_millis = oil_metrics->time_millis;

    public_udp_context_t *public_packet_context = get_public_packet_context();
    if (!public_packet_context->time_synchronized && modbus_rtu_context->time_sync_waits < TIME_SYNC_WAITS_MAX) {
        modbus_rtu_context->time_sync_waits++;
        return -1;
    }
    
    // lock context
    int lock_ret = public_udp_context_lock();
    if (lock_ret != 0) {
        return -1;
    }

    if (public_packet_context != NULL) {
        public_packet_t *public_packet = &public_packet_context->public_packet;

        struct tm time_struct = { 0 };
        time_struct.tm_year = public_packet->year - 1900;
        time_struct.tm_mon = public_packet->month - 1;
        time_struct.tm_mday = public_packet->date;
        time_struct.tm_hour = public_packet->hour;
        time_struct.tm_min = public_packet->min;
        time_struct.tm_sec = public_packet->second;
        time_t time_seconds = mktime(&time_struct);
        int64_t public_packet_time_millis = ((int64_t) time_seconds) * 1000LL + public_packet->millisecond;

        memcpy(oil_metrics_storage.flags, public_packet->flags, 4);
        oil_metrics_storage.public_packet_time_millis = public_packet_time_millis;

        oil_metrics_storage.speed = public_packet->speed;
        oil_metrics_storage.km_post = public_packet->km_post;

        oil_metrics_storage.train_serial_no = public_packet->train_serial_no;
        oil_metrics_storage.temperature_outside = public_packet->temperature_outside;
        oil_metrics_storage.locomotive_double_heading_status = public_packet->locomotive_double_heading_status;
        oil_metrics_storage.carriage_no = public_packet->carriage_no;
        strcpy(oil_metrics_storage.train_no, public_packet->train_no);
    }

    // unlock
    public_udp_context_unlock();

    // save to files
    for(int i=0; i<modbus_rtu_context->modbus_rtu_config->file_storage_config_count; i++) {
        file_storage_context_t *file_storage_context = modbus_rtu_context->file_storage_contexts + i;
        file_storage_write(file_storage_context, (char *) &oil_metrics_storage, sizeof(oil_metrics_storage_t));
    }

    // convert flags to string
    char flags[16];
    sprintf(flags, "%02X%02X%02X%02X", oil_metrics_storage.flags[3] & 0xFF, oil_metrics_storage.flags[2] & 0xFF, oil_metrics_storage.flags[1] & 0xFF, oil_metrics_storage.flags[0] & 0xFF);

    // save to mysql
    char sql_buffer[512];
    memset(sql_buffer, 0, sizeof(sql_buffer));
    sprintf(sql_buffer,
            "insert into oil_sensor_metrics "
            "(device_name, device_address, temperature, water_activity, ppm, viscosity, density, dielectric_constant, flags, public_packet_time, speed, km_post, train_serial_no, temperature_outside, locomotive_double_heading_status, carriage_no, train_no, create_time) "
            "values('%s', %d, %f, %f, %f, %f, %f, %f,  '%s', %lld, %f, %d, %d, %d, %d, %d, '%s', %lld)",

            oil_metrics_storage.device_name,
            oil_metrics_storage.device_address,

            oil_metrics_storage.temperature,
            oil_metrics_storage.water_activity,
            oil_metrics_storage.ppm,
            oil_metrics_storage.viscosity,
            oil_metrics_storage.density,
            oil_metrics_storage.dielectric_constant,

            flags,
            oil_metrics_storage.public_packet_time_millis,
            oil_metrics_storage.speed,
            oil_metrics_storage.km_post,
            oil_metrics_storage.train_serial_no,
            oil_metrics_storage.temperature_outside,
            oil_metrics_storage.locomotive_double_heading_status,
            oil_metrics_storage.carriage_no,
            oil_metrics_storage.train_no,

            oil_metrics_storage.time_millis);

    // execute SQL asynchronous
    return execute_sql_async(sql_buffer);
}

static int collect_modbus_rtu_data_collect_module(
    modbus_rtu_context_t *modbus_rtu_context,
    modbus_rtu_t *modbus_rtu,
    modbus_rtu_device_t *device,
    collect_module_metrics_t *collect_module_metrics) {
    // request data through MODBUS RTU protocol
    m_log(M_LOG_INFO, "Reading registers from collect moduble '%s' by MODBUS RTU protocol", device->device_name);
    char data[256];
    int modbus_rtu_request_ret = modbus_rtu_read_registers(
        modbus_rtu_context,
        modbus_rtu,
        device,
        0x00,
        5 * 2,
        5 * 4,
        data);
    if (modbus_rtu_request_ret < 0) {
        return -1;
    }

    float abrasion = read_float(data, 0);
    float temperature = read_float(data, 4);
    float pressure = read_float(data, 8);
    float board_temperature = read_float(data, 12);
    int error_code = read_int(data, 16);

    collect_module_metrics->device_name = device->device_name;
    collect_module_metrics->device_address = device->device_address;

    collect_module_metrics->abrasion = abrasion;
    collect_module_metrics->temperature = temperature;
    collect_module_metrics->pressure = pressure;
    collect_module_metrics->board_temperature = board_temperature;
    collect_module_metrics->error_code = error_code;

    return 0;
}

static int save_collect_module_metrics(
    modbus_rtu_context_t *modbus_rtu_context,
    collect_module_metrics_t *collect_module_metrics) {

    char sql_buffer[512];
    memset(sql_buffer, 0, sizeof(sql_buffer));
    sprintf(sql_buffer,
            "insert into collect_module_metrics "
            "(device_name, device_address, abrasion, temperature, pressure, board_temperature, error_code, create_time) "
            "values('%s', %d, %f, %f, %f, %f, %d, %lld)",

            collect_module_metrics->device_name,
            collect_module_metrics->device_address,

            collect_module_metrics->abrasion,
            collect_module_metrics->temperature,
            collect_module_metrics->pressure,
            collect_module_metrics->board_temperature,
            collect_module_metrics->error_code,

            collect_module_metrics->time_millis);

    // execute SQL asynchronous
    return execute_sql_async(sql_buffer);
}

static int modbus_rtu_read_registers(
    modbus_rtu_context_t *modbus_rtu_context,
    modbus_rtu_t *modbus_rtu,
    modbus_rtu_device_t *device,

    int start_reg_address,
    int read_reg_count,
    int reg_data_length,

    char *data_buffer) {
    int uart_fd = modbus_rtu_context->uart_fd;

    fd_set file_descriptor_set;
    FD_ZERO(&file_descriptor_set);
    FD_SET(uart_fd, &file_descriptor_set);
    fd_set saved_file_descriptor_set = file_descriptor_set;

    // address and function code
    char request_data_buffer[256];
    memset(request_data_buffer, 0, sizeof(request_data_buffer));
    int i = 0;
    request_data_buffer[i++] = device->device_address;
    request_data_buffer[i++] = 0x04;

    // reg address
    request_data_buffer[i++] = (start_reg_address >> 8) & 0xFF;
    request_data_buffer[i++] = start_reg_address & 0xFF;

    // reg count
    request_data_buffer[i++] = (read_reg_count >> 8) & 0xFF;
    request_data_buffer[i++] = read_reg_count & 0xFF;

    // checksum CRC-16, note that this is little-end
    unsigned short crc16 = crc16_digest(request_data_buffer, i);
    request_data_buffer[i++] = crc16 & 0xFF;
    request_data_buffer[i++] = (crc16 >> 8) & 0xFF;

    // send
    int write_total = 0;
    while (write_total < i) {
        int written = write(uart_fd, request_data_buffer + write_total, i - write_total);
        write_total += written;
    }

    // the MODBUS RTU device might not response to us
    // so here we set an timeout here
    const int response_data_buffer_length = 256;

    int error = 1;                                                // 0: no error, 1: error
    int expected_data_length = reg_data_length;                   // from the incoming parameters
    int expected_response_length = 3 + expected_data_length + 2;  // 1 device address, 1 func code, 1 data length, n data length, 2 CRC bytes
    char response_data_buffer[response_data_buffer_length];       // read data buffer
    int response_data_exception = 0;                              // 0: no exception, 1: exception occurred, need skip rest data and sleep for a while
    int read_total = 0;                                           // read count

    m_log(M_LOG_INFO, "Expected response length = %d", expected_response_length);

    // clear buffer
    memset(request_data_buffer, 0, sizeof(request_data_buffer));

    while (!shutdown_flag) {
        // read the response, use select
        file_descriptor_set = saved_file_descriptor_set;
        FD_SET(uart_fd, &file_descriptor_set);

        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 200 * 1000; // timeout for 50ms

        // select read events
        int select_ret = select(uart_fd + 1, &file_descriptor_set, NULL, NULL, &timeout);
        if (select_ret < 0) {
            // error
            m_log(M_LOG_ERROR, "Error occurs while select read events of '%s'", modbus_rtu->uart_name);
            break;
        } else if (select_ret == 0) {
            // timeout
            m_log(M_LOG_ERROR, "Timeout while select read events of '%s', will abandon this request.", modbus_rtu->uart_name);
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
                
                int buffer_free_length = response_data_buffer_length - read_total;
                int to_read = available_to_read > buffer_free_length ? buffer_free_length: available_to_read;
                if (available_to_read > buffer_free_length) {
                    response_data_exception = 1;
                    m_log(M_LOG_WARN, "Frame too large and exceed buffer, expected='%d', actual='%d'", expected_data_length, read_total + to_read);
                    break;
                }

                int read_bytes = read(uart_fd, response_data_buffer + read_total, to_read);
                if (read_bytes >= 0) {
                    read_total += read_bytes;
                }

                if (read_total >= 1) {
                    // check device address
                    int address = response_data_buffer[0] & 0xFF;
                    if (address != device->device_address) {
                        m_log(M_LOG_ERROR, "Response device address mismatched, expected '%d', actual '%d'", device->device_address, address);
                        response_data_exception = 1;
                        break;
                    }
                }
                if (read_total >= 2) {
                    // check function code
                    int func_code = response_data_buffer[1] & 0xFF;
                    if (func_code == 0x83) {
                        m_log(M_LOG_ERROR, "Device '%s' return an error code '%d'", device->device_name, func_code);
                        response_data_exception = 1;
                        break;
                    }
                    if (func_code != 0x04) {
                        m_log(M_LOG_ERROR, "Response function code mismatched, expected '%d', actual '%d'", 0x04, func_code);
                        response_data_exception = 1;
                        break;
                    }
                }

                if (read_total >= 3) {
                    // check data length
                    int data_length = response_data_buffer[2] & 0xFF;
                    if (data_length != expected_data_length) {
                        m_log(M_LOG_ERROR, "Reponse data length mismatched, expected '%d', actual '%d'", expected_data_length, data_length);
                        response_data_exception = 1;
                        break;
                    }
                }

                if (read_total >= expected_response_length) {
                    error = 0;
                    break;  // read done
                }
            }
        }
    }

    if (error == 0) {
        // compare the CRC16 checksum
        int crc16_lb = response_data_buffer[expected_response_length - 2];
        int crc16_hb = response_data_buffer[expected_response_length - 1];
        int crc16 = ((crc16_hb << 8) & 0xFF00) | (crc16_lb & 0xFF);
        int calc_crc16 = crc16_digest(response_data_buffer, expected_response_length - 2);
        if (crc16 != calc_crc16) {
            char data_str_buffer[1024];
            memset(data_str_buffer, 0, sizeof(data_str_buffer));
            sprintf_bytes(data_str_buffer, response_data_buffer, expected_response_length);
            m_log(M_LOG_ERROR, "CRC-16 checksum mismatch, calculated: '%04X', actual: '%04X', data: %s", calc_crc16, crc16, data_str_buffer);
            error = 1;
        }

        if (error == 0) {
            // return the data
            int start = 3;
            for (int i = 0; i < expected_data_length; i++) {
                data_buffer[i] = response_data_buffer[start + i];
            }
        }
    }
    
    if (response_data_exception || read_total > expected_response_length) {
        // char data_str_buffer[1024];
        // memset(data_str_buffer, 0, sizeof(data_str_buffer));
        // sprintf_bytes(data_str_buffer, response_data_buffer, read_total);
        // m_log(M_LOG_ERROR, "data: %s", data_str_buffer);
        m_log(M_LOG_ERROR, "Skip rest data for device '%s'", device->device_name);
        // skip data loop
        while (1) {
            struct timeval skip_timeout;
            skip_timeout.tv_sec = 0;
            skip_timeout.tv_usec = 50 * 1000;  // timeout for 250ms

            // read the data, use select
            file_descriptor_set = saved_file_descriptor_set;
            FD_SET(uart_fd, &file_descriptor_set);

            // select read events
            int select_ret = select(uart_fd + 1, &file_descriptor_set, NULL, NULL, &skip_timeout);
            if (select_ret < 0) {
                // error
                m_log(M_LOG_ERROR, "Skipping data and error occurs while select read events of '%s'", modbus_rtu->uart_name);
                break;
            } else if (select_ret == 0) {
                // timeout
                m_log(M_LOG_ERROR, "Skipping data and timeout while select read events of '%s', will abandon this request.", modbus_rtu->uart_name);
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

                    char dummy_buffer[256];
                    memset(dummy_buffer, 0, sizeof(dummy_buffer));
                    read(uart_fd, dummy_buffer, available_to_read);
                }
            }
        }
    }

    if (error != 0) {
        return -1;
    }
    return 0;
}

static int init_uart(int uart_fd, modbus_rtu_t *modbus_rtu) {
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
    } else if(modbus_rtu->stop_bits == 2) {
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
    uart_termios.c_cflag |= CREAD | CLOCAL | get_termios_data_bits_flag(modbus_rtu->data_bits);
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

static int get_termios_data_bits_flag(int data_bits) {
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
