#include <stdio.h>
#include "mbed.h"
#include "rtos.h"
#include "am2315.h"
#include "wind.h"
#include "rain.h"
#include "gps.h"
#include "light.h"

#include "crc.h"

DigitalOut led1(LED1);

DigitalIn x_on(PB_8);
DigitalOut x_sby(PC_12,0);

// Thread gps_thread;

Wind wind(PC_2, PA_4);
Rain rain(PC_3);
Light light(PA_1);
GPS gps(PA_9, PA_10); //tx rx

AM2315 th(PB_14, PB_13);

Serial xbee_uart(PC_10, PC_11, 115200);

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

// void gps_thread_fn() {
//     static char str[128];
//     printf("GPS TEST\n");
//     snprintf(str, sizeof("GPS TEST\n"), "GPS TEST\n");

//     while(true) {
//         uint8_t *line = gps.readline();
//         printf("%s\n", line);
//         snprintf(str, sizeof(str), "%s\n", line);
//         packet_tx(strlen(str), str);
//         ThisThread::sleep_for(10);
//     }
// }

int main() {

    // gps_thread.start(gps_thread_fn);

    static char str[128];

    while (true) {
        led1 = !led1;

        float wind_speed = wind.read_kph();
        float wind_dir = wind.read_dir();
        float rain_mm = rain.read_mm();
        float light_level = light.read();
        th.read();

        printf("wspeed: %1.2f kph @ %3.1f\n", wind_speed, wind_dir);
        printf("Rain: %f mm\n", rain_mm);
        printf("Light: %f\n", light_level);

        xbee_enable();
        snprintf(str, sizeof(str), "wspeed: %1.2f kph @ %3.1f\n", wind_speed, wind_dir);
        packet_tx(strlen(str), str);
        wait(0.01);
        snprintf(str, sizeof(str), "Rain: %f mm\n", rain_mm);
        packet_tx(strlen(str), str);
        wait(0.01);
        snprintf(str, sizeof(str), "Light: %f\n", light_level);
        packet_tx(strlen(str), str);
        wait(0.01);
        printf("t: %0.2f C, h:%0.2f %%RH\n", th.celsius, th.humidity);
        snprintf(str, sizeof(str), "t: %0.2f C, h:%0.2f %%RH\n", th.celsius, th.humidity);
        packet_tx(strlen(str), str);
        wait(0.01);
        xbee_disable();


        wait(10);
    }
}

