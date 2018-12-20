#include "wind.h"
#include "mbed.h"

typedef struct {
    uint16_t voltage;
    uint16_t direction;
} wind_dir_t;

// Lookup table to get wind direction from voltage
// Auto-generated with winddircalc.py
// VDD3.30= R_PU=4700 R_S=2000
static const wind_dir_t wind_dir_lut[] = {
    {1229, 1125},
    {1271, 675},
    {1337, 900},
    {1472, 1575},
    {1640, 1350},
    {1780, 2025},
    {1984, 1800},
    {2195, 225},
    {2407, 450},
    {2586, 2475},
    {2687, 2250},
    {2833, 3375},
    {2946, 0},
    {3033, 2925},
    {3131, 3150},
    {3384, 2700},
    {0 ,0}};


Wind::Wind(PinName speed_pin, PinName dir_pin)
    : _interrupt(speed_pin), _dir_pin(dir_pin) {
    _interrupt.mode(PullUp);
    _interrupt.rise(callback(this, &Wind::tick));
    timer.start();

}

void Wind::tick() {
    _ticks++;
}

float Wind::read_kph() {
    int32_t tmp_ticks = _ticks;
    _ticks = 0;

    float speed = (tmp_ticks * 2.4f)/timer.read();
    timer.reset();

    return speed;
}

float Wind::read_dir() {
    float mv = 0;
    int16_t direction = -1;

    mv = _dir_pin.read() * 3300;
    for (uint8_t dir = 0; dir < sizeof(wind_dir_lut)/sizeof(wind_dir_t); dir++) {
        if (mv < wind_dir_lut[dir].voltage) {
            direction = wind_dir_lut[dir].direction;
            break;
        }

        // TODO add invalid check
    }
    return direction/10.0f;
}

