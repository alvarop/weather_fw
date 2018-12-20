#ifndef __GPS_H__
#define __GPS_H__

#include "mbed.h"

class GPS {
public:
    GPS(PinName tx_pin, PinName rx_pin);
    uint8_t * readline();

private:
    Serial uart;
    uint8_t gps_linebuff[256];
};

#endif