#include "demo1_modbus_source.h"

#include "demo1_modbus.h"

int demo1_modbus_source_init(demo1_modbus_source_t *source,
                             void *transport_context,
                             demo1_modbus_transport_write_fn write,
                             demo1_modbus_transport_read_fn read,
                             uint8_t slave_id,
                             uint16_t start_register,
                             uint16_t register_count) {
    if (source == NULL || transport_context == NULL || write == NULL || read == NULL ||
        slave_id == 0U || register_count == 0U ||
        register_count > DEMO1_ANALOG_CHANNEL_COUNT) {
        return -1;
    }

    source->transport_context = transport_context;
    source->write = write;
    source->read = read;
    source->slave_id = slave_id;
    source->start_register = start_register;
    source->register_count = register_count;
    return 0;
}

int demo1_modbus_source_read_tick(void *context,
                                  uint64_t timestamp_us,
                                  demo1_tick_sample_t *out_sample) {
    demo1_modbus_source_t *source = (demo1_modbus_source_t *)context;
    uint8_t request[DEMO1_MODBUS_READ_REQUEST_LENGTH];
    uint8_t response[64];
    uint16_t registers[DEMO1_ANALOG_CHANNEL_COUNT];
    uint16_t register_count = 0U;
    size_t response_length = 0U;
    int request_length;

    (void)timestamp_us;

    if (source == NULL || out_sample == NULL || source->write == NULL || source->read == NULL) {
        return -1;
    }

    request_length = demo1_modbus_build_read_request(source->slave_id,
                                                     source->start_register,
                                                     source->register_count,
                                                     request,
                                                     sizeof(request));
    if (request_length <= 0) {
        return -1;
    }
    if (source->write(source->transport_context, request, (size_t)request_length) != 0) {
        return -2;
    }
    if (source->read(source->transport_context, response, sizeof(response), &response_length) != 0) {
        return -3;
    }
    if (demo1_modbus_parse_read_response(source->slave_id,
                                         response,
                                         response_length,
                                         registers,
                                         source->register_count,
                                         &register_count) != 0) {
        return -4;
    }
    if (register_count != source->register_count) {
        return -4;
    }
    if (demo1_modbus_registers_to_tick_sample(registers, register_count, out_sample) != 0) {
        return -4;
    }
    return 0;
}
