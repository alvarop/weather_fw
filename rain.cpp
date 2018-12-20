#include "mbed.h"
#include "rain.h"

Rain::Rain(PinName pin) : _interrupt(pin) {
    _interrupt.mode(PullUp);
    _interrupt.rise(callback(this, &Rain::tick));
}

void Rain::tick() {
    _ticks++;
}

float Rain::read_mm() {
    return _ticks * 0.2794;
}
