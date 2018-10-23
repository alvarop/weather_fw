#include <stdio.h>
#include "mbed.h"
#include "rtos.h"

DigitalOut led1(LED1);

Thread gps_thread;

class Light {
public:
    Light(PinName pin) : _light(pin) {
    }

    float read() {
        return _light.read();
    }

private:
    AnalogIn _light;
};

class Rain {
public:
    Rain(PinName pin) : _interrupt(pin) {
        _interrupt.mode(PullUp);
        _interrupt.rise(callback(this, &Rain::tick));
    }

    void tick() {
        _ticks++;
    }

    float read_mm() {
        return _ticks * 0.2794;
    }

private:
    InterruptIn _interrupt;
    volatile uint32_t _ticks;
};

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


class Wind {
public:
    Wind(PinName speed_pin, PinName dir_pin)
        : _interrupt(speed_pin), _dir_pin(dir_pin) {
        _interrupt.mode(PullUp);
        _interrupt.rise(callback(this, &Wind::tick));
        timer.start();

    }

    void tick() {
        _ticks++;
    }

    float read_kph() {
        int32_t tmp_ticks = _ticks;
        _ticks = 0;

        float speed = (tmp_ticks * 2.4f)/timer.read();
        timer.reset();

        return speed;
    }

    float read_dir() {
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

private:
    InterruptIn _interrupt;
    AnalogIn _dir_pin;
    volatile uint32_t _ticks;
    Timer timer;
};

class GPS {
public:
    GPS(PinName tx_pin, PinName rx_pin)
        : uart(tx_pin, rx_pin) {
    }

    uint8_t * readline() {
        uint16_t index = 0;
        bool scanning = true;

        while(scanning) {
            gps_linebuff[index] = uart.getc();

            if (gps_linebuff[index] == '\n') {
                gps_linebuff[index] = 0;
                scanning = false;
            }

            index++;

            if (index >= (sizeof(gps_linebuff) - 1)) {
                index = 0;
            }
    }

    return gps_linebuff;
}


private:
    Serial uart;
    uint8_t gps_linebuff[256];
};

Wind wind(PC_2, PA_4);
Rain rain(PC_3);
Light light(PA_1);
GPS gps(PA_9, PA_10); //tx rx

void gps_thread_fn() {
    while(true) {
        uint8_t *line = gps.readline();
        printf("\"%s\"\n", line);
        ThisThread::sleep_for(10);
    }
}

int main() {
    printf("GPS TEST\n");
    gps_thread.start(gps_thread_fn);
    while (true) {
        led1 = !led1;
        wait(2);
        printf("wspeed: %1.2f kph @ %3.1f\n", wind.read_kph(), wind.read_dir());
        printf("Rain: %f mm\n", rain.read_mm());
        printf("Light: %f\n", light.read());
    }
}

