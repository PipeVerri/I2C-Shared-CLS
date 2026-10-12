#pragma once
#include <Arduino.h>

enum State {
  START,
  READING,
  WRITING,
  AWAITING_ACK,
  SENDING_ACK,
  RELEASING_ACK_ON_NEXT_FALL,
  SENDING_NACK,
  STOPPING,
  IDLE,
};

enum StartMode {
    START_READING,
    START_WRITING
};

class Lane {
    private:
        State state = IDLE;
        const uint8_t sda_pin;
        uint8_t sequence;
        uint8_t sequence_idx;
        bool is_sda_released;
        bool finish_reading_with_ack;
        bool has_just_read;

        void lower_sda();
        void release_sda();
        bool read_sda();
        void ll_start(uint8_t raw_address);

    public:
        bool successful_write;

        Lane(uint8_t sda_pin);
        // Sends a start signal and the address to the device
        // The address doesn't have to include the R/W byte
        void start(uint8_t address, StartMode mode);
        void stop();
        // Writes a byte to an already-started connection
        void write(uint8_t message);
        // Reads a byte from an already started connection
        // will_read_again indicates if other bytes are expected to follow the one being read or not
        void read(bool will_read_again);
        // Used by the controller
        void on_scl_raise();
        void on_scl_fall();
        // Status checks
        bool busy() { return state != IDLE; }
        uint8_t get_last_read_byte(); // Doesn't modify this
};

void exception(String message);