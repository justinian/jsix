#pragma once
/// \file serial.h
/// Declarations related to serial ports.
#include <stdint.h>

#include <util/bip_buffer.h>

class serial_port
{
public:
    /// Constructor.
    /// \arg port   The IO address of the serial port
    serial_port(uint16_t port);

    serial_port();

    void write(char c);
    char read();

    void handle_interrupt();

private:
    bool m_writing;
    uint16_t m_port;
    util::bip_buffer m_out_buffer;
    util::bip_buffer m_in_buffer;

    void do_read();
    void do_write();
    void handle_error(uint16_t reg, uint8_t value);
};

extern serial_port &g_com1;
