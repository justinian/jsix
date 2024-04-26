#pragma once
/// \file serial.h
/// Declarations related to serial ports.
#include <stdint.h>

#include <util/bip_buffer.h>
#include <util/spinlock.h>

namespace j6 {
    class channel;
}

class serial_port
{
public:
    /// Constructor.
    /// \arg port     The IO address of the serial port
    /// \art channel  The channel to send bytes over
    serial_port(uint16_t port, j6::channel &channel);

    size_t write(const char *str, size_t len);
    char read();

    void handle_interrupt();
    void do_write();
    void do_read();

private:
    bool m_writing;
    uint16_t m_port;
    j6::channel &m_chan;
    util::spinlock m_lock;

    void handle_error(uint16_t reg, uint8_t value);
};
