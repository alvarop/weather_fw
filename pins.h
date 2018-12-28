#ifndef __PINS_H__
#define __PINS_H__

#ifndef BOARD_VERSION
#define BOARD_VERSION 2
#endif

#if BOARD_VERSION == 1

#define LED_PIN             LED1

#define XBEE_ON_PIN         PB_8
#define XBEE_nSBY_PIN       PC_12

#define XBEE_UART_TX        PC_11
#define XBEE_UART_RX        PC_10

#define WIND_SPEED_PIN      PC_2
#define WIND_DIR_PIN        PA_4
#define RAIN_PIN            PC_3
#define LIGHT_PIN           PA_1
#define GPS_TX_PIN          PA_10
#define GPS_RX_PIN          PA_9

#define I2C_SDA_PIN         PB_14
#define I2C_SCL_PIN         PB_13

#elif BOARD_VERSION == 2

#define LED_PIN             LED1

#define XBEE_ON_PIN         PC_9
#define XBEE_nSBY_PIN       PC_8
#define XBEE_nRST_PIN       PC_6

#define XBEE_UART_TX        PC_11
#define XBEE_UART_RX        PC_10

#define WIND_SPEED_PIN      PC_12
#define WIND_DIR_PIN        PA_1
#define RAIN_PIN            PD_2
#define LIGHT_PIN           PA_4
#define GPS_TX_PIN          PA_10
#define GPS_RX_PIN          PA_9

#define GPS_FIX_PIN         PC_4
#define GPS_PPS_PIN         PC_2
#define GPS_nSBY_PIN        PC_0
#define GPS_nRST_PIN        PC_3

#define I2C_SDA_PIN         PB_9
#define I2C_SCL_PIN         PB_8

#else
#error Invalid board version
#endif

#endif
