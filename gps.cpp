#include "gps.h"
#include "mbed.h"
#include <string.h>
#include "fifo.h"
#include "nmea.h"
#include "gpgga.h"

#define FIFO_BUFF_SIZE  (2048)

static uint8_t inBuff[FIFO_BUFF_SIZE];

void GPS::gps_thread_fn(GPS *gps) {

    while(true) {

        // TODO - wait on semaphore instead of looping until ready
        // TODO - add timeout if no fix in x minutes
        if (gps->gps_fix_flag == true) {

            char *sentence = (char *)gps->readline();

            if(strlen(sentence) > 0) {
                // Pointer to struct containing the parsed data. Should be freed manually.
                nmea_s *data;

                // Parse...
                data = nmea_parse(sentence, strlen(sentence), 0);
                if(data != NULL) {
                    // print GPGGA data
                    if (NMEA_GPGGA == data->type) {
                        nmea_gpgga_s *gpgga = (nmea_gpgga_s *) data;
                        printf("GPGGA[%d, %02d] %d %f %c %d %f %c\n",
                            gpgga->quality,
                            gpgga->n_satellites,
                            gpgga->longitude.degrees,
                            gpgga->longitude.minutes,
                            (char) gpgga->longitude.cardinal,
                            gpgga->latitude.degrees,
                            gpgga->latitude.minutes,
                            (char) gpgga->latitude.cardinal
                            );

                        gps->lon_degrees = gpgga->longitude.degrees;
                        gps->lon_minutes = gpgga->longitude.minutes;
                        gps->lon_cardinal = (char)gpgga->longitude.cardinal;
                        gps->lat_degrees = gpgga->latitude.degrees;
                        gps->lat_minutes = gpgga->latitude.minutes;
                        gps->lat_cardinal = (char)gpgga->latitude.cardinal;

                        // Disable RX irq
                        gps->uart.attach(NULL);

                        // Put GPS to sleep
                        gps->gps_sby.write(0);

                        // TODO - put thread to sleep
                    }

                    nmea_free(data);

                }
            }
        }

        ThisThread::sleep_for(50);
    }
}

GPS::GPS(PinName tx_pin, PinName rx_pin, uint32_t baud_rate,
            PinName gps_fix_pin, PinName gps_nrst_pin, PinName gps_sby_pin)
    : uart(tx_pin, rx_pin, baud_rate),
        gps_fix(gps_fix_pin),
        gps_nrst(gps_nrst_pin, 1),
        gps_sby(gps_sby_pin, 1) {

    fifoInit(&rxFifo, FIFO_BUFF_SIZE, inBuff);

    gps_fix.rise(callback(gps_fix_irq, this));
    gps_fix_flag = false;

    lat_degrees = 0;
    lat_minutes = 0.0f;
    lat_cardinal = ' ';
    lon_degrees = 0;
    lon_minutes = 0.0f;
    lon_cardinal = ' ';
}

bool GPS::position_ready() {
    return (gps_fix_flag && lon_cardinal != ' ');
}

void GPS::start() {
    gps_thread.start(callback(gps_thread_fn, this));
}

void GPS::uart_irq(GPS *gps) {
    fifoPush(&gps->rxFifo, gps->uart.getc());
}

void GPS::gps_fix_irq(GPS *gps) {
    gps->gps_fix_flag = true;

    //disable fix irq
    gps->gps_fix.rise(NULL);

    // Setup uart RX irq
    gps->uart.attach(callback(GPS::uart_irq, gps));
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

