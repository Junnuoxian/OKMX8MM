#include <string.h>

#include "demo3_m4_modbus.h"

typedef struct {
    uint8_t request[8];
    uint8_t response[3u + (DEMO3_MODBUS_REGISTER_COUNT * 2u) + 2u];
    size_t response_length;
} mock_bus_t;

static int mock_write(void *context, const uint8_t *data, size_t length)
{
    mock_bus_t *bus = (mock_bus_t *)context;
    if (length != sizeof(bus->request)) {
        return -1;
    }
    memcpy(bus->request, data, length);
    return 0;
}

static int mock_read(void *context, uint8_t *data, size_t capacity, size_t *out_length)
{
    mock_bus_t *bus = (mock_bus_t *)context;
    if (capacity < bus->response_length) {
        return -1;
    }
    memcpy(data, bus->response, bus->response_length);
    *out_length = bus->response_length;
    return 0;
}

int main(void)
{
    mock_bus_t bus;
    demo3_sample_t sample;
    uint16_t registers[DEMO3_MODBUS_REGISTER_COUNT] = {0};
    size_t i;

    memset(&bus, 0, sizeof(bus));
    bus.response[0] = 1u;
    bus.response[1] = 0x03u;
    bus.response[2] = DEMO3_MODBUS_REGISTER_COUNT * 2u;
    registers[0] = 0u;
    registers[1] = 1234u;
    registers[20] = 3u;
    registers[21] = 0u;
    registers[22] = 88u;
    registers[23] = DEMO3_SAMPLE_VALID;
    registers[24] = 0u;
    registers[25] = 9u;
    for (i = 0u; i < DEMO3_MODBUS_REGISTER_COUNT; ++i) {
        bus.response[3u + (i * 2u)] = (uint8_t)(registers[i] >> 8u);
        bus.response[4u + (i * 2u)] = (uint8_t)(registers[i] & 0xFFu);
    }
    bus.response_length = sizeof(bus.response);
    {
        uint16_t crc = demo3_crc16_modbus(bus.response, (uint32_t)(bus.response_length - 2u));
        bus.response[bus.response_length - 2u] = (uint8_t)(crc & 0xFFu);
        bus.response[bus.response_length - 1u] = (uint8_t)(crc >> 8u);
    }

    if (demo3_m4_modbus_read_sample(&bus, mock_write, mock_read, 1u, 0u, &sample) != 0) {
        return 1;
    }
    if (bus.request[0] != 1u || bus.request[1] != 0x03u ||
        bus.request[4] != 0u || bus.request[5] != DEMO3_MODBUS_REGISTER_COUNT ||
        sample.analog[0] != 1234 || sample.digital_bits != 3u ||
        sample.speed_rpm != 88u || sample.sequence != 9u) {
        return 2;
    }
    bus.response[bus.response_length - 1u] ^= 0x01u;
    if (demo3_m4_modbus_read_sample(&bus, mock_write, mock_read, 1u, 0u, &sample) != -5) {
        return 3;
    }
    return 0;
}
