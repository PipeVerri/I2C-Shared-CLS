# SoftI2C_Shared_CLS

Allows the use of multiple non-programmable I2C devices with the same address while using around half as many GPIO pins.

It does that by sharing SCL across devices and keeping a single SDA per device.

## Wiring

* Connect all SCL pins together to one GPIO (`scl_pin`).
* Connect each device's SDA to its own GPIO (one `Lane` per device).

N devices with the same address = 1 SCL pin + N SDA pins. All lanes share one clock; idle lanes ignore ticks, so you only need each lane to be idle before queueing its next operation.

## Basic Usage

```cpp
#include "SoftI2C_Shared_CLS.h"

Lane lanes[] = { Lane(13), Lane(12) };
Bus<2> myBus(11, lanes); // shared SCL on pin 11

void loop() {
  // 1. START + address (7-bit) + R/W mode
  lanes[0].start(0x36, START_WRITING); lanes[1].start(0x36, START_WRITING);
  while (lanes[0].busy() || lanes[1].busy()) { myBus.step(); }

  // 2. Write register address
  lanes[0].write(0x0C); lanes[1].write(0x0C);
  while (lanes[0].busy() || lanes[1].busy()) { myBus.step(); }

  // 3. Repeated START in read mode
  lanes[0].start(0x36, START_READING); lanes[1].start(0x36, START_READING);
  while (lanes[0].busy() || lanes[1].busy()) { myBus.step(); }

  // 4. Read bytes. `true` = more bytes follow (ACK), `false` = last byte (NACK)
  lanes[0].read(true); lanes[1].read(true);
  while (lanes[0].busy() || lanes[1].busy()) { myBus.step(); }
  uint8_t high0 = lanes[0].get_last_read_byte();
  uint8_t high1 = lanes[1].get_last_read_byte();

  lanes[0].read(false); lanes[1].read(false);
  while (lanes[0].busy() || lanes[1].busy()) { myBus.step(); }
  uint8_t low0 = lanes[0].get_last_read_byte();
  uint8_t low1 = lanes[1].get_last_read_byte();

  // 5. Optional STOP
  lanes[0].stop(); lanes[1].stop();
  while (lanes[0].busy() || lanes[1].busy()) { myBus.step(); }
}
```

See `examples/Read_as5600.ino` for a full example reading two AS5600 encoders at address `0x36`.

Pattern is always per lane: queue operation -> `while(lane.busy()) bus.step()` -> read result with `get_last_read_byte()` / `successful_write`. Queueing all lanes together before stepping (as above) is just the simplest way to do it; you can also stagger lanes since `bus.step()` advances all busy lanes and idle ones ignore the clock.

## API

### `Lane(uint8_t sda_pin)`
One device / one SDA line.

* `start(address, mode)` — `address` is 7-bit (R/W bit added automatically). `mode`: `START_READING` or `START_WRITING`.
* `write(uint8_t byte)` — write a byte on an already-started transaction.
* `read(bool will_read_again)` — read a byte. Use `true` if you will read more bytes after this one.
* `get_last_read_byte()` — call right after a `read()` finishes. Returns the byte.
* `stop()` — send STOP condition. Only needed if sharing the bus with another master.
* `busy()` — `true` while an operation is pending on that lane. Call `bus.step()` until that lane is idle before queueing its next operation.
* `successful_write` — `true` if the slave ACKed the last address/byte.

### `Bus<N>(scl_pin, lanes, us_delay = 5)`
Coordinates the shared clock.

* `N` must match the size of your `lanes[]` array, e.g. `Bus<2>`.
* `step()` — toggles SCL once and advances all lanes. Call in a loop until every lane reports `!busy()`.
* `us_delay` — half-clock delay in microseconds. Default 5 (~100 kHz).

## License

GPLv3 — see `LICENSE`.
