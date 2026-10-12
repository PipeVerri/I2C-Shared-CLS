#pragma once
#include "lane.h"
#include "bus.h"

struct LaneStep {
    void (*action)(Lane&, uint8_t); // A pointer to a function (named action) that takes a Lane pointer and an integer
    void (*on_done)(Lane&, uint8_t); // What to run after the action is done
};


template <uint8_t N>
class Bitset {
    private:
        uint8_t bytes[(N + 7) / 8] = {0}; // ceil(n/8) bytes
    
    public:
        void set(uint8_t i) {
            uint8_t byte_num = i >> 3; // The result of i / 8
            bitSet(bytes[byte_num], i & 0x7); // i & 0x7 is i % 8
        }

        void reset(uint8_t i) {
            uint8_t byte_num = i >> 3; // The result of i / 8
            bitClear(bytes[byte_num], i & 0x7); // i & 0x7 is i % 8
        }

        bool query(uint8_t i) {
            uint8_t byte_num = i >> 3; // The result of i / 8
            uint8_t bit_location_mask = 1UL << (i & 0x7);
            return (bytes[byte_num] & bit_location_mask) != 0;
        }
};

template <size_t N_STEPS, uint8_t N_LANES>
Bitset<N_LANES> run_steps(LaneStep (&steps)[N_STEPS], Lane* (&lanes)[N_LANES], Bus<N_LANES>& bus) {
    // lanes is a reference to an array of Lane pointers
    Bitset<N_LANES> failed_lanes;

    for (size_t s = 0; s < N_STEPS; s++) {
        for (uint8_t l = 0; l < N_LANES; l++) {
            if (!failed_lanes.query(l)) {
                steps[s].action(*lanes[l], l);
            }
        }

        bool busy = true;
        int c = 0;
        while (busy) {
            busy = false;
            for (uint8_t l = 0; l < N_LANES; l++) {
                if (!failed_lanes.query(l) && lanes[l]->busy()) {
                    busy = true;
                    break;
                }
            }
            if (!busy) break;
            bus.step();
        }

        for (size_t l = 0; l < N_LANES; l++) {
            if (!lanes[l]->successful_write) {
                failed_lanes.set(l);
            } else {
                steps[s].on_done(*lanes[l], l);
            }
        }
    }

    return failed_lanes;
}