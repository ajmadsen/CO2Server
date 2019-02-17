#include <ACROBOTIC_SSD1306.h>

#include "mhz19b.h"

namespace MHZ19B
{

uint8_t calculateChecksum(uint8_t *packet)
{
    char cksum = 0x0;
    for (uint8_t i = 1; i < 8; ++i)
    {
        cksum += packet[i];
    }
    return ~cksum + 1;
}

void setRange(const PpmRange range)
{
    uint8_t width = 2000;
    switch (range)
    {
    case PPM_5000:
        width = 5000;
        break;
    default: // PPM_2000
        break;
    };

    uint8_t packet[9] = {0xff, 0x01,
                         0x99,
                         width >> 8, width & 0xff,
                         0x00, 0x00, 0x00,
                         0x00};
    packet[8] = calculateChecksum(packet);

    Serial.write(packet, 8);
}

int16_t readCO2()
{
    uint8_t packet[9] = {0xff, 0x01,
                         0x86,
                         0x00, 0x00, 0x00, 0x00, 0x00,
                         0x00};
    packet[8] = calculateChecksum(packet);

    // flush receive buffer
    while (Serial.read() > 0)
        ;

    Serial.write(packet, sizeof packet);
    Serial.flush();

    memset(packet, 1, sizeof packet);

    Serial.setTimeout(10000);
    const size_t read = Serial.readBytes(packet, sizeof packet);
    if (read <= 0)
    {
        return E_TimedOut;
    }

    if (packet[8] != calculateChecksum(packet))
    {
        return E_CksumFail;
    }

    return ((uint16_t)packet[2] << 8) | packet[3];
}

} // namespace MHZ19B