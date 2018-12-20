#include "mbed.h"
#include "light.h"

Light::Light(PinName pin) : _light(pin) {
}

float Light::read() {
    return _light.read();
}
