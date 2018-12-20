#ifndef __LIGHT_H__
#define __LIGHT_H__

#include "mbed.h"

class Light {
public:
    Light(PinName pin);

    float read();

private:
    AnalogIn _light;
};

#endif