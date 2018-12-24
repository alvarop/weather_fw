#include "gps.h"
#include "mbed.h"
#include <string.h>
#include "fifo.h"

#define FIFO_BUFF_SIZE  (2048)

static uint8_t inBuff[FIFO_BUFF_SIZE];

GPS::GPS(PinName tx_pin, PinName rx_pin)
    : uart(tx_pin, rx_pin) {

    fifoInit(&rxFifo, FIFO_BUFF_SIZE, inBuff);

    // Setup uart RX irq
    uart.attach(callback(GPS::irq, this));
}

void GPS::irq(GPS *gps_obj) {
    fifoPush(&gps_obj->rxFifo, gps_obj->uart.getc());
}

// Read NMEA line/sentence if available
uint8_t * GPS::readline() {
    uint16_t len = 0;
    gps_linebuff[0] = 0;

    // Find start of sentence and discard other characters
    while (fifoSize(&rxFifo) && fifoPeek(&rxFifo, 0) != '$') {
        fifoPop(&rxFifo);
    }

    if (fifoSize(&rxFifo) && fifoPeek(&rxFifo, 0) == '$') {
        // Find end of sentence
        for(uint32_t byte = 0; byte < fifoSize(&rxFifo); byte++) {
            if(fifoPeek(&rxFifo, byte) == '\n') {
                len = byte + 1;
                break;
            }
        }
    }

    if(len < (sizeof(gps_linebuff) - 1)) {
        uint16_t index = 0;
        while(len--) {
            gps_linebuff[index++] = fifoPop(&rxFifo);
        }
        gps_linebuff[index] = 0;
    } else {
        // Line too long, remove from fifo
        while(len--) {
            fifoPop(&rxFifo);
        }
    }

    return gps_linebuff;
}

