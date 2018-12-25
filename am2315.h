// Code from https://os.mbed.com/users/pici/code/AM2315/

#ifndef __AM2315_H__
#define __AM2315_H__

#include "mbed.h"

#define AM2315_ADDR             0xB8
#define AM2315_REG_READ         0x03

class AM2315
{
    public:
        AM2315(I2C &i2c_obj);
        bool read();

        float celsius;
        float humidity;
    private:
        I2C &i2c;
};

#endif