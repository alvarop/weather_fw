#ifndef __BMP280_H__
#define __BMP280_H__

#include "mbed.h"

#define BMP280_ADDR             (0x76 << 1)
#define BMP280_ADDR_ALT         (0x77 << 1)
#define BMP280_DIG_T1_ADDR      (0x88)
#define BMP280_CHIP_ID_ADDR     (0xD0)
#define BMP280_SOFT_RESET_ADDR  (0xE0)
#define BMP280_STATUS_ADDR      (0xF3)
#define BMP280_CTRL_MEAS_ADDR   (0xF4)
#define BMP280_CONFIG_ADDR      (0xF5)
#define BMP280_PRES_ADDR        (0xF7)
#define BMP280_TEMP_ADDR        (0xFA)

#define BMP280_OS_NONE          (0x00)
#define BMP280_OS_1X            (0x01)
#define BMP280_OS_2X            (0x02)
#define BMP280_OS_4X            (0x03)
#define BMP280_OS_8X            (0x04)
#define BMP280_OS_16X           (0x05)

#define BMP280_SLEEP_MODE       (0x00)
#define BMP280_FORCED_MODE      (0x01)
#define BMP280_NORMAL_MODE      (0x03)

#define BMP280_ODR_0_5_MS       (0x00)
#define BMP280_ODR_62_5_MS      (0x01)
#define BMP280_ODR_125_MS       (0x02)
#define BMP280_ODR_250_MS       (0x03)
#define BMP280_ODR_500_MS       (0x04)
#define BMP280_ODR_1000_MS      (0x05)
#define BMP280_ODR_2000_MS      (0x06)
#define BMP280_ODR_4000_MS      (0x07)

#define BMP280_FILTER_OFF       (0x00)
#define BMP280_FILTER_COEFF_2   (0x01)
#define BMP280_FILTER_COEFF_4   (0x02)
#define BMP280_FILTER_COEFF_8   (0x03)
#define BMP280_FILTER_COEFF_16  (0x04)

typedef struct {
    uint16_t t1;
    int16_t t2;
    int16_t t3;
    uint16_t p1;
    int16_t p2;
    int16_t p3;
    int16_t p4;
    int16_t p5;
    int16_t p6;
    int16_t p7;
    int16_t p8;
    int16_t p9;
} __attribute__((packed)) bmp280_cal_t;


class BMP280
{
public:

    BMP280(I2C &i2c_obj, char i2c_addr = BMP280_ADDR);

    void read();
    void init(void);
    float getTemperature() const { return temperature; };
    float getPressure() const { return pressure; };

private:

    I2C             &i2c;
    char            addr;
    bmp280_cal_t    cal;
    float           temperature;
    float           pressure;
};

#endif
