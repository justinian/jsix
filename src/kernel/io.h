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

}

const uint16_t COM1 = 0x03f8;
