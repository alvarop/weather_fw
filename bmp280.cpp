#include "mbed.h"
#include "bmp280.h"

BMP280::BMP280(I2C &i2c_obj, char i2c_addr)
    :i2c(i2c_obj),
    addr(i2c_addr) {}

void BMP280::init() {
    char cmd[4];

    cmd[0] = BMP280_CTRL_MEAS_ADDR;
    cmd[1] = (BMP280_OS_1X << 2) |
              (BMP280_OS_1X << 5) |
              BMP280_NORMAL_MODE; // 1x oversampling, normal mode
    i2c.write(addr, cmd, 2);

    cmd[0] = BMP280_CONFIG_ADDR;
    cmd[1] = (BMP280_FILTER_OFF << 2) |
              (BMP280_ODR_1000_MS << 5);
    i2c.write(addr, cmd, 2);

    cmd[0] = BMP280_DIG_T1_ADDR;
    i2c.write(addr, cmd, 1);
    i2c.read(addr, (char *)&cal, sizeof(bmp280_cal_t));
}

void BMP280::read() {
    uint32_t temp_raw;
    uint32_t press_raw;
    int32_t t_fine;
    float var1, var2;
    char cmd[7];

    // Read both temperature and pressure registers
    cmd[0] = BMP280_PRES_ADDR;
    i2c.write(addr, cmd, 1);
    i2c.read(addr, &cmd[1], 6);

    press_raw = (cmd[1] << 12) | (cmd[2] << 4) | (cmd[3] >> 4);
    temp_raw = (cmd[4] << 12) | (cmd[5] << 4) | (cmd[6] >> 4);

    var1 = (((float) temp_raw) / 16384.0 - ((float) cal.t1) / 1024.0) * ((float) cal.t2);
    var2 = ((((float) temp_raw) / 131072.0 - ((float) cal.t1) / 8192.0) * (((float) temp_raw) / 131072.0 - ((float) cal.t1) / 8192.0)) * ((float) cal.t3);
    t_fine = (int32_t) (var1 + var2);

    temperature = (var1 + var2) / 5120.0;

    var1 = ((float) t_fine / 2.0) - 64000.0;
    var2 = var1 * var1 * ((float) cal.p6) / 32768.0;
    var2 = var2 + var1 * ((float) cal.p5) * 2.0;
    var2 = (var2 / 4.0) + (((float) cal.p4) * 65536.0);
    var1 = (((float) cal.p3) * var1 * var1 / 524288.0 + ((float) cal.p2) * var1) / 524288.0;
    var1 = (1.0 + var1 / 32768.0) * ((float) cal.p1);

    pressure = 1048576.0 - (float) press_raw;

    if (var1 != 0) {
      pressure = (pressure - (var2 / 4096.0)) * 6250.0 / var1;
      var1 = ((float) cal.p9) * pressure * pressure / 2147483648.0;
      var2 = pressure * ((float) cal.p8) / 32768.0;
      pressure = pressure + (var1 + var2 + ((float) cal.p7)) / 16.0;
    } else {
      pressure = 0;
    }
}
