#include "SoftI2C_Shared_CLS.h"

// Each I2C device is a lane. The pins for their SDA's are passed
Lane lanes[] = { Lane(13), Lane(12) };
Bus<2> myBus(11, lanes); // The Bus coordinates all the lanes, the SCL pin and the array of lanes are passed
uint16_t angles[2] = {0};

void setup() {
  Serial.begin(9600);
}

void loop() {
    // Start by sending a START signal to them
    lanes[0].start(0x36, START_WRITING) ; lanes[1].start(0x36, START_WRITING); // Pass the 7 bit address and if it's R/W(sets the last bit)
    while (lanes[0].busy() || lanes[1].busy()) { myBus.step(); } // myBus.step() controls the clock and calls the lanes' on_scl_raise and on_scl_fall methods

    // Send a write to move the register to the RAW_ANGLE address
    lanes[0].write(0x0C) ; lanes[1].write(0x0C);
    while (lanes[0].busy() || lanes[1].busy()) { myBus.step(); }

    // Restart on read mode
    lanes[0].start(0x36, START_READING) ; lanes[1].start(0x36, START_READING);
    while (lanes[0].busy() || lanes[1].busy()) { myBus.step(); }

    // Read the first 11:7 bits of the 12 bit angle
    uint16_t angles[2] = {0};
    // Makes them release SDA, read the slave's byte and then sends an ACK
    // Their argument is if more bytes/read operations are expected (end with ACK) or if it's the last byte expected (end with NACK)
    lanes[0].read(true) ; lanes[1].read(true);
    while (lanes[0].busy() || lanes[1].busy()) { myBus.step(); }

    // Write the read bits to the angles array
    // The 0x0F mask is applied just in case
    angles[0] = ((lanes[0].get_last_read_byte() & 0x0F) << 8) ; angles[1] = ((lanes[0].get_last_read_byte() & 0x0F) << 8)

    // Read the rest of the 7:0 bits
    // This time, read will be called with 'false' because it's the last byte being read in this operation
    lanes[0].read(false) ; lanes[1].read(false);
    while (lanes[0].busy() || lanes[1].busy()) { myBus.step(); }

    // Write the rest of the bits
    angles[0] += lanes[0].get_last_read_byte() ; angles[1] += lanes[1].get_last_read_byte();

    // Show the raw angles to serial
    Serial.print("Lane 1 angle: ");
    Serial.print(angles[0]);
    Serial.print(" Lane 2 angle: ");
    Serial.print(angles[1]);

    // Closing the transaction isn't neccesary here, only shown as an example. Only necessary if you're sharing the I2C line with another master
    lanes[0].stop() ; lanes[1].stop();
}