#include "demo1_wheel_board_source.h"

#include <string.h>

#include "demo1_modbus.h"

static int wheel_registers_to_sample(const uint16_t *registers,
                                     uint16_t register_count,
                                     demo1_tick_sample_t *out_sample) {
    if (registers == NULL || out_sample == NULL ||
        register_count != DEMO1_WHEEL_MODBUS_REGISTER_COUNT) {
        return -1;
    }

    memset(out_sample, 0, sizeof(*out_sample));
    out_sample->analog_channel_count = DEMO1_WHEEL_AI_CHANNEL_COUNT;
    out_sample->valid_mask = (uint16_t)((1U << DEMO1_WHEEL_AI_CHANNEL_COUNT) - 1U);
    for (uint8_t index = 0U; index < DEMO1_WHEEL_AI_CHANNEL_COUNT; ++index) {
        out_sample->analog[index] = (int16_t)registers[index];
    }
    out_sample->digital_bits =
        (uint8_t)(registers[DEMO1_WHEEL_MODBUS_DI_REGISTER_INDEX] &
                  ((1U << DEMO1_WHEEL_DI_CHANNEL_COUNT) - 1U));
    out_sample->hall_pulse_delta[0] =
        registers[DEMO1_WHEEL_MODBUS_SPEED_REGISTER_INDEX];
    return 0;
}

int demo1_wheel_board_source_init(demo1_wheel_board_source_t *source,
                                  void *transport_context,
                                  demo1_modbus_transport_write_fn write,
                                  demo1_modbus_transport_read_fn read,
                                  uint8_t slave_id,
                                  uint16_t start_register) {
    if (source == NULL || transport_context == NULL || write == NULL || read == NULL ||
        slave_id == 0U) {
        return -1;
    }

    source->transport_context = transport_context;
    source->write = write;
    source->read = read;
    source->slave_id = slave_id;
    source->start_register = start_register;
    return 0;
}

int demo1_wheel_board_source_read_tick(void *context,
                                       uint64_t timestamp_us,
                                       demo1_tick_sample_t *out_sample) {
    demo1_wheel_board_source_t *source = (demo1_wheel_board_source_t *)context;
    uint8_t request[DEMO1_MODBUS_READ_REQUEST_LENGTH];
    uint8_t response[64];
    uint16_t registers[DEMO1_WHEEL_MODBUS_REGISTER_COUNT];
    uint16_t register_count = 0U;
    size_t response_length = 0U;
    int request_length;

    (void)timestamp_us;

    if (source == NULL || out_sample == NULL || source->write == NULL || source->read == NULL) {
        return -1;
    }

    request_length = demo1_modbus_build_read_request(source->slave_id,
                                                     source->start_register,
                                                     DEMO1_WHEEL_MODBUS_REGISTER_COUNT,
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
                                         DEMO1_WHEEL_MODBUS_REGISTER_COUNT,
                                         &register_count) != 0) {
        return -4;
    }
    if (wheel_registers_to_sample(registers, register_count, out_sample) != 0) {
        return -4;
    }
    return 0;
}
