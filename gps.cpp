#include "gps.h"
#include "mbed.h"


GPS::GPS(PinName tx_pin, PinName rx_pin)
    : uart(tx_pin, rx_pin) {
}

uint8_t * GPS::readline() {
    uint16_t index = 0;
    bool scanning = true;

    while(scanning) {
        gps_linebuff[index] = uart.getc();

        if (gps_linebuff[index] == '\n') {
            gps_linebuff[index] = 0;
            scanning = false;
        }

        index++;

        if (index >= (sizeof(gps_linebuff) - 1)) {
            index = 0;
        }
    }

    return gps_linebuff;
}

