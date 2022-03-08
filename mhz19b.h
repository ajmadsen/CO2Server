#ifndef MHZ19B_H
#define MHZ19B_H

namespace MHZ19B
{

    enum PpmRange
    {
        PPM_2000,
        PPM_5000
    };

    class device_t
    {
    public:
        device_t();
        device_t(Stream *stream);

        void reset(Stream *stream);

        void setRange(const PpmRange range);
        void setABC(bool enabled);
        void setZero();

        int16_t read(unsigned long timeout = 5000L);

    private:
        Stream *stream;
    };

    const int16_t E_TimedOut = -1;
    const int16_t E_CksumFail = -2;

} // namespace MHZ19B

#endif // MHZ19B_H