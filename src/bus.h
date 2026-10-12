#pragma once
#include <Arduino.h>
#include "lane.h"

template <uint8_t N> // Let the compiler figure out N
class Bus {
    private:
        uint8_t scl_pin;
        Lane (&lanes)[N];
        uint8_t us_delay;
        bool scl_state;

        void lower_scl() {
            if (scl_state == true) {
                pinMode(scl_pin, OUTPUT);
                digitalWrite(scl_pin, LOW);
                scl_state = false;
            }
        }

        void release_scl() {
            if (scl_state == false) {
                pinMode(scl_pin, INPUT_PULLUP);
                scl_state = true;
            }
        }
    
    public:
        Bus(uint8_t _scl_pin, Lane (&_lanes)[N], uint8_t _us_delay = 5):
            scl_pin(_scl_pin),
            lanes(_lanes),
            us_delay(_us_delay),
            scl_state(false)
        {
            release_scl();
        }

        void step() {
            if (scl_state == true) {
                lower_scl();
                for (uint8_t i = 0; i < N; i++) {
                    lanes[i]->on_fall();
                }
            } else {
                release_scl();
                for (uint8_t i = 0; i < N; i++) {
                    lanes[i]->on_raise();
                }
            }
            delayMicroseconds(us_delay);
        }
};