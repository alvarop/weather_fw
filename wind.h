#ifndef __WIND_H__
#define __WIND_H__

#include "mbed.h"

class Wind {
public:
    Wind(PinName speed_pin, PinName dir_pin);

    void tick();
    float read_kph();
    float read_dir();

private:
    InterruptIn _interrupt;
    AnalogIn _dir_pin;
    volatile uint32_t _ticks;
    Timer timer;
};

#endif