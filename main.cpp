#include <stdio.h>
#include "mbed.h"
#include "rtos.h"
#include "am2315.h"
#include "bmp280.h"
#include "wind.h"
#include "rain.h"
#include "gps.h"
#include "light.h"
#include "crc.h"
#include "pins.h"

DigitalOut led1(LED_PIN);

DigitalIn x_on(XBEE_ON_PIN);
DigitalOut x_sby(XBEE_nSBY_PIN,0);
Serial xbee_uart(XBEE_UART_RX, XBEE_UART_TX, 115200);

Wind wind(WIND_SPEED_PIN, WIND_DIR_PIN);
Rain rain(RAIN_PIN);
Light light(LIGHT_PIN);
GPS gps(GPS_RX_PIN, GPS_TX_PIN, 115200,
        GPS_FIX_PIN, GPS_nRST_PIN, GPS_nSBY_PIN);

I2C i2c(I2C_SDA_PIN, I2C_SCL_PIN);

AM2315 th(i2c);
BMP280 bmp280(i2c);

typedef struct {
    uint16_t start;
    uint16_t len;
} __attribute__((packed)) packet_header_t;

typedef struct {
    uint16_t crc;
} __attribute__((packed)) packet_footer_t;

#define MAX_DATA_LEN 90
#define MAX_PACKET_SIZE (sizeof(packet_header_t) + sizeof(packet_footer_t) + MAX_DATA_LEN)
#define PACKET_START 0xAA55

static uint8_t packet_tx_buff[MAX_PACKET_SIZE];

void xbee_enable() {
    x_sby = 0;
}

void xbee_disable() {
    x_sby = 1;
}

int32_t packet_tx(uint16_t len, void *data) {
    int32_t rval = 0;

    do {
        if(len > MAX_DATA_LEN) {
            rval = -1;
            break;
        }
        crc_t crc;

        packet_header_t *header = (packet_header_t *)packet_tx_buff;
        packet_footer_t *footer = (packet_footer_t *)((uint8_t *)&header[1] + len);

        header->start = PACKET_START;
        header->len = len;

        memcpy((void*)&header[1], data, len);

        crc = crc_init();
        crc = crc_update(crc, header, header->len + sizeof(packet_header_t));
        crc = crc_finalize(crc);

        footer->crc = crc;

        xbee_uart.write(
            packet_tx_buff,
            header->len + sizeof(packet_footer_t) + sizeof(packet_header_t),
            NULL);

    } while(0);

    return rval;
}

static uint32_t *uid = (uint32_t *)(0x1FFF7590);

int main() {
    static char str[128];

    gps.start();

    wait(1);

    printf("UID:%08lX\n", uid[0] ^ uid[1] ^ uid[2]);

    // Dummy reads and initializations
    th.read();
    bmp280.init();

    while (true) {
        led1 = !led1;

        float wind_speed = wind.read_kph();
        float wind_dir = wind.read_dir();
        float rain_mm = rain.read_mm();
        float light_level = light.read() * 3.3f * 5.67f; // Hijacking to measure 12V battery voltage
        th.read();
        bmp280.read();

        printf("wspeed: %1.2f kph @ %3.1f\n", wind_speed, wind_dir);
        printf("Rain: %f mm\n", rain_mm);
        printf("t: %0.2f C, h:%0.2f %%RH\n", th.celsius, th.humidity);
        printf("t2: %0.2f p:%0.2f\n", bmp280.getTemperature(), bmp280.getPressure()/100.0);
        printf("Light: %f\n", light_level);

        xbee_enable();
        wait(0.01);
        snprintf(str, sizeof(str),
            "ws:%1.2f\n"
            "wd:%3.1f\n"
            "ra:%0.3f\n"
            "te:%0.2f\n"
            "hu:%0.2f\n"
            "pr:%0.2f\n"
            "li:%0.3f\n",
            wind_speed,
            wind_dir,
            rain_mm,
            th.celsius,
            th.humidity,
            (bmp280.getPressure() / 100.0), // convert to hPa
            light_level
            );
        packet_tx(strlen(str), str);
        wait(0.01);
        xbee_disable();

        wait(60);
    }
}

