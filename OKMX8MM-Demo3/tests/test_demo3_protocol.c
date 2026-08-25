#include <stdint.h>

#include "demo3_protocol.h"

int main(void)
{
    static const uint8_t request[] = {0x01u, 0x03u, 0x00u, 0x00u, 0x00u, 0x0Au};

    /* Modbus CRC is transmitted low byte first: C5 CD. */
    return demo3_crc16_modbus(request, sizeof(request)) == 0xCDC5u ? 0 : 1;
}
