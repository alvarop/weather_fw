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

#define WX_SAMPLE_PERIOD_S (60)

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

#if BOARD_VERSION == 1
BMP280 bmp280(i2c, BMP280_ADDR_ALT);
#else
BMP280 bmp280(i2c, BMP280_ADDR);
#endif

typedef enum {
    PACKET_TYPE_DATA = 1,
    PACKET_TYPE_GPS = 2
} packet_type_t;

typedef struct {
    uint32_t    uid;
    uint8_t     type;
    float       wind_speed;
    float       wind_dir;
    float       rain;
    float       temperature;
    float       humidity;
    float       temperatre_in;
    float       pressure;
    float       light;
    float       battery;
} __attribute__((packed)) weather_data_packet_t;

// TODO
typedef struct {
    uint32_t    uid;
    uint8_t     type;
    int32_t     lat_degrees;
    double      lat_minutes;
    char        lat_cardinal;
    int32_t     lon_degrees;
    double      lon_minutes;
    char        lon_cardinal;
} __attribute__((packed)) weather_gps_packet_t;

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

    uint32_t gps_packet_tx = 1;

    gps.start();

    wait(1);

    // Dummy reads and initializations
    th.read();
    bmp280.init();

    while (true) {
        weather_data_packet_t packet;

        led1 = !led1;

        packet.uid = uid[0] ^ uid[1] ^ uid[2];
        packet.type = PACKET_TYPE_DATA;

        packet.wind_speed = wind.read_kph();
        packet.wind_dir = wind.read_dir();
        packet.rain = rain.read_mm();
        packet.light = 0; // Used for battery right now
        packet.battery = light.read() * 3.3f * 5.67f; // Hijacking to measure 12V battery voltage

        th.read();
        bmp280.read();

        packet.temperature = th.celsius;
        packet.humidity = th.humidity;

        packet.temperatre_in = bmp280.getTemperature();
        packet.pressure = bmp280.getPressure()/100.0;

        printf("wspeed: %1.2f kph @ %3.1f\n", packet.wind_speed, packet.wind_dir);
        printf("Rain: %f mm\n", packet.rain);
        printf("t: %0.2f C, h:%0.2f %%RH\n", packet.temperature, packet.humidity);
        printf("t2: %0.2f p:%0.2f\n", packet.temperatre_in, packet.pressure);
        printf("Light: %f\n", packet.light);
        printf("Batt: %f\n", packet.battery);

        xbee_enable();
        wait(0.01);
        packet_tx(sizeof(weather_data_packet_t), (void*)&packet);
        wait(0.01);

        if (gps.position_ready() && (--gps_packet_tx == 0)) {

            // Only send gps packet every hour
            gps_packet_tx = 60;

            weather_gps_packet_t gps_packet;
            gps_packet.uid = uid[0] ^ uid[1] ^ uid[2];
            gps_packet.type = PACKET_TYPE_GPS;

            gps_packet.lat_degrees = gps.lat_degrees;
            gps_packet.lat_minutes = gps.lat_minutes;
            gps_packet.lat_cardinal = gps.lat_cardinal;
            gps_packet.lon_degrees = gps.lon_degrees;
            gps_packet.lon_minutes = gps.lon_minutes;
            gps_packet.lon_cardinal = gps.lon_cardinal;

            packet_tx(sizeof(weather_gps_packet_t), (void*)&gps_packet);
            wait(0.01);
        }
        xbee_disable();

        wait(WX_SAMPLE_PERIOD_S);
    }
}

