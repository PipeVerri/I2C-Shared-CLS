#include <Arduino.h>
#include "lane.h"

// Raw control
void Lane::lower_sda() {
  if (is_sda_released == true) {
      pinMode(sda_pin, OUTPUT);
      digitalWrite(sda_pin, false);
      is_sda_released = false;
  }
}

void Lane::release_sda() {
  if (is_sda_released == false) {
    pinMode(sda_pin, INPUT_PULLUP);
    is_sda_released = true;
  }
}

bool Lane::read_sda() {
  if (is_sda_released == false) {
    exception("SDA Cannot be read if not released beforehand");
    return false;
  } else {
    return digitalRead(sda_pin);
  }
}

// Constructor
Lane::Lane(uint8_t _sda_pin): 
  sda_pin(_sda_pin),
  sequence(0),
  sequence_idx(0),
  is_sda_released(false),
  finish_reading_with_ack(false) {
    release_sda();
}

// I2C interfaces
void Lane::ll_start(uint8_t raw_address) {
  write(raw_address);
  state = START;
}

void Lane::start(uint8_t address, StartMode mode) {
  // Receives a 7 bit address
  uint8_t raw_address = address << 1;

  if (mode == START_READING) {
    raw_address = raw_address | 0x01;
  }
  release_sda();
  ll_start(raw_address);
} 

void Lane::write(uint8_t message) {
  if (state != IDLE) {
    exception("Cant start/write while state isn't idle (did you run bus.step()?)");
  }

  sequence = message;
  sequence_idx = 8;
  state = WRITING;
  has_just_read = false;
}

void Lane::read(bool will_read_again) {
  if (state != IDLE) {
    exception("Cant read while state isn't idle (did you run bus.step()?)");
  }
  sequence = 0;
  sequence_idx = 8;
  state = READING;
  finish_reading_with_ack = will_read_again;
  has_just_read = true;
}


void Lane::stop() {
  if (state != IDLE) {
    exception("Can't stop in non-idle state (did you run bus.step()?)");
  }
  // Lower SDA on the next clock cycle, and increase it mid-cycle
  // There's no risk of running into the increase condition too early 
  // because on_lower runs first because the previous state was idle(with SCL released)
  state = STOPPING;
}

// Controller interfaces
void Lane::on_raise() {
  switch (state) {
    case START:
      lower_sda();       // Drop SDA while SCL raises
      state = WRITING;  // Then, SCL will drop and the first bit will be written
      break;

    case WRITING:
      // Check if we've sent the whole sequence
      if (sequence_idx == 0) {
        state = AWAITING_ACK;  // The next fall, SDA is released and then, on the next rise, we check if it has been lowered
      }

      break;  // Only write when SCL is low
    
    case READING: { // brackets restrict the scope of bit to the READING case
      bool bit = read_sda();
      sequence <<= 1;
      sequence |= bit;
      sequence_idx--;
      // When the whole byte is read, then I'm sending an ACK on the 9th pulse
      if (sequence_idx == 0) {
        if (finish_reading_with_ack) {
          state = SENDING_ACK;
        } else {
          // Just send a NACK, SDA is already released
          // This state is set instead of idle so the extra clock cycle happens
          state = SENDING_NACK;
        }
      }
      break;
    }

    case AWAITING_ACK:
      state = IDLE;
      // Check if the slave lowered SDA
      if (read_sda() == false) {  // SDA is low, ACK received
        successful_write = true;
      } else {
        successful_write = false;
      }
      break;
    
    case SENDING_ACK:
      if (finish_reading_with_ack) {
        lower_sda();
      }
      // Leave state as SENDING_ACK so SDA is released for some (possible) next reading on the next fall 
      break;
    
    case SENDING_NACK:
      // The 9th cycle rise has now happened. It's time to go idle
      state = IDLE;
      break;
    
    case RELEASING_ACK_ON_NEXT_FALL:
      break; // Ignore this case. Release SDA on the next fall
    
    case STOPPING:
      release_sda();
      state = IDLE;
      break;
    
    case IDLE:
      break; // Clock can tick while it's IDLE when another lane is still doing stuff
    
    default:
      Serial.println(state);
      exception("The above case wasn't supposed to happen on_raise");
      break;
  }
}

void Lane::on_fall() {
  switch (state) {
    case START:
      break;  // Only send the start sequence when SCL is high

    case WRITING:
      // Write the next sequence digit, from most to least significant
      if (sequence & 0x80) {  // If the current bit is 1: (0x80 = 1000 0000)
        //Serial.print("1");
        release_sda();
      } else {
        //Serial.print("0");
        lower_sda();
      }

      // Shift the sequence
      sequence_idx--;
      sequence <<= 1;

      // The check if we've sent the whole sequence is done in the onRaise: WRITING
      break;
    
    case READING:
      if (sequence_idx == 0) {
        state = SENDING_ACK;
      }
      break;

    case AWAITING_ACK:
      //Serial.println("");
      // We release SDA, and check on the next raise if it has been lowered
      release_sda();
      break;
    
    case SENDING_ACK:
      // Send the ACK on the 9th pulse, release it on the next fall
      lower_sda();
      state = RELEASING_ACK_ON_NEXT_FALL;
      break;
    
    case SENDING_NACK:
      break; // SDA is already released, but wait until the 9th cycle raise to actually go idle
    
    case RELEASING_ACK_ON_NEXT_FALL:
      release_sda();
      state = IDLE;
      break;
    
    case STOPPING:
      // Release it mid-cycle next
      lower_sda();
      break;

    case IDLE:
      break; // Clock can tick while it's IDLE when another lane is still doing stuff

    default:
      Serial.println(state);
      exception("The above case wasn't supposed to happen on_fall");
      break;
  }
}

// User interfaces
uint8_t Lane::get_last_read_byte() {
  if (has_just_read) {
    if (state != IDLE) {
      exception("You must wait until the read operation finishes(did you run tick?)");
    } else {
      return sequence;
    }
  } else {
    exception("You must read the byte right after doing the read operation, without any other operations in between");
  }
}

// Exceptions
void exception(String message) {
  Serial.println(message);
  while (true) { }
}