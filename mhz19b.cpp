#include <Stream.h>
#include <StreamDev.h>
#include <Arduino.h>

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

    device_t::device_t() : device_t(&devnull) {}
    device_t::device_t(Stream *stream) { reset(stream); }

    void device_t::reset(Stream *stream)
    {
        if (!stream)
            stream = &devnull;
        this->stream = stream;
    }

    void device_t::setRange(const PpmRange range)
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

        stream->write(packet, sizeof packet);
    }

    void device_t::setABC(bool enabled)
    {
        uint8_t packet[9] = {0xff, 0x01,
                             0x79,
                             enabled ? 0xA0 : 0x00,
                             0x00, 0x00, 0x00, 0x00,
                             0x00};
        packet[8] = calculateChecksum(packet);

        stream->write(packet, sizeof packet);
    }

    void device_t::setZero()
    {
        uint8_t packet[9] = {0xff, 0x01,
                             0x87,
                             0x00, 0x00, 0x00, 0x00, 0x00,
                             0x00};
        packet[8] = calculateChecksum(packet);

        stream->write(packet, sizeof packet);
    }

    int16_t device_t::read(unsigned long timeout)
    {
        uint8_t packet[9] = {0xff, 0x01,
                             0x86,
                             0x00, 0x00, 0x00, 0x00, 0x00,
                             0x00};
        packet[8] = calculateChecksum(packet);

        // flush receive buffer
        while (stream->available() > 0)
            stream->read();

        stream->write(packet, sizeof packet);
        stream->flush();

        memset(packet, 1, sizeof packet);

        stream->setTimeout(timeout);
        const size_t read = stream->readBytes(packet, sizeof packet);

#if 0
        printf("read %d bytes\n", read);
        for (int i = 0; i < read; i++)
            printf("0x%02x ", packet[i]);
        printf("\n");
#endif

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