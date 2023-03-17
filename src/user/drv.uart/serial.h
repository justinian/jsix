#pragma once
/// \file serial.h
/// Declarations related to serial ports.
#include <stdint.h>

#include <util/bip_buffer.h>
#include <util/spinlock.h>

class serial_port
{
public:
    /// Constructor.
    /// \arg port   The IO address of the serial port
    serial_port(uint16_t port,
            size_t in_buffer_len, uint8_t *in_buffer,
            size_t out_buffer_len, uint8_t *out_buffer);

    size_t write(const char *str, size_t len);
    char read();

    void handle_interrupt();
    void do_write();

private:
    bool m_writing;
    uint16_t m_port;
    util::bip_buffer m_out_buffer;
    util::bip_buffer m_in_buffer;
    util::spinlock m_lock;

    void do_read();
    void handle_error(uint16_t reg, uint8_t value);
};
