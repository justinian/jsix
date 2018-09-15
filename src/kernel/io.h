#pragma once

#include <stdint.h>

extern "C" {

/// Read a byte from an IO port.
/// \arg port  The address of the IO port
/// \returns   One byte read from the port
uint8_t inb(uint16_t port);

/// Write a byte to an IO port.
/// \arg port  The addres of the IO port
/// \arg val   The byte to write
void outb(uint16_t port, uint8_t val);

/// Pause briefly by doing IO to port 0x80
/// \arg times  Number of times to delay by writing
void io_wait(unsigned times = 1);

}

const uint16_t COM1 = 0x03f8;
