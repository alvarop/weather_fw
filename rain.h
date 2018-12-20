#ifndef __RAIN_H__
#define __RAIN_H__

#include "mbed.h"

class Rain {
public:
    Rain(PinName pin);
    void tick();
    float read_mm();

private:
    InterruptIn _interrupt;
    volatile uint32_t _ticks;
};

#endif