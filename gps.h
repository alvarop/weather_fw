#ifndef __GPS_H__
#define __GPS_H__

#include "mbed.h"
#include "fifo.h"

class GPS {
public:
    GPS(PinName tx_pin, PinName rx_pin);
    uint8_t * readline();
private:
    static void irq(GPS *gah);
    uint8_t gps_linebuff[256];
    fifo_t rxFifo;
    RawSerial uart;
};

#endif
