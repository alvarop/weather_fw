#ifndef __GPS_H__
#define __GPS_H__

#include "mbed.h"
#include "fifo.h"

class GPS {
public:
    GPS(PinName tx_pin, PinName rx_pin, uint32_t baud_rate,
        PinName gps_fix_pin, PinName gps_nrst_pin, PinName gps_sby_pin);
    uint8_t * readline();
    void start();
private:
    static void uart_irq(GPS *gps);
    static void gps_thread_fn(GPS *gps);
    static void gps_fix_irq(GPS *gps);
    bool gps_fix_flag;
    uint8_t gps_linebuff[256];
    fifo_t rxFifo;
    RawSerial uart;
    InterruptIn gps_fix;
    DigitalOut gps_nrst;
    DigitalOut gps_sby;
    Thread gps_thread;
};

#endif
