#pragma once
/// \file panic_serial.h
/// Non-interrupt-driven serial 'driver' for panic handling
#include <stdint.h>

namespace panic {

class serial_port
{
public:
    /// Constructor.
    /// \arg port   The IO address of the serial port
    serial_port(uint16_t port);

    void write(const char *s);

private:
    uint16_t m_port;
};

constexpr uint16_t COM1 = 0x03f8;

} // namespace panic
