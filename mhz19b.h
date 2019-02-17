#ifndef MHZ19B_H
#define MHZ19B_H

namespace MHZ19B
{

enum PpmRange
{
    PPM_2000,
    PPM_5000
};

void setRange(const PpmRange range);
int16_t readCO2();

const int16_t E_TimedOut = -1;
const int16_t E_CksumFail = -2;

} // namespace MHZ19B

#endif // MHZ19B_H