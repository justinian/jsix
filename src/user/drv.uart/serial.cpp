#include <string.h>
#include "io.h"
#include "serial.h"

constexpr size_t fifo_size = 64;

// register offsets
constexpr uint16_t THR = 0; // Write
constexpr uint16_t RBR = 0; // Read
constexpr uint16_t IER = 1;
constexpr uint16_t FCR = 2; // Write
constexpr uint16_t IIR = 2; // Read
constexpr uint16_t LCR = 3;
constexpr uint16_t MCR = 4;
constexpr uint16_t LSR = 5;
constexpr uint16_t MSR = 6;

constexpr uint16_t DLL = 0; // DLAB == 1
constexpr uint16_t DLH = 1; // DLAB == 1

serial_port::serial_port(uint16_t port,
        size_t in_buffer_len, uint8_t *in_buffer,
        size_t out_buffer_len, uint8_t *out_buffer) :
    m_writing(false),
    m_port(port),
    m_out_buffer(out_buffer, out_buffer_len),
    m_in_buffer(in_buffer, in_buffer_len)
{
    outb(port + IER, 0x00);  // Disable all interrupts
    outb(port + LCR, 0x80);  // Enable the Divisor Latch Access Bit
    outb(port + DLL, 0x01);  // Divisor low byte: 1 (115200 baud)
    outb(port + DLH, 0x00);  // Divisor high byte
    outb(port + LCR, 0x03);  // 8-N-1, diable DLAB
    outb(port + FCR, 0xe7);  // Clear and enable FIFO, enable 64 byte, 56 byte trigger
    outb(port + MCR, 0x0b);  // Data terminal ready, Request to send, aux output 2 (irq enable)
    outb(port + IER, 0x03);  // Enable interrupts

    // Clear out pending interrupts
    handle_interrupt();
}

inline bool read_ready(uint16_t port) { return (inb(port + LSR) & 0x01) != 0; }
inline bool write_ready(uint16_t port) { return (inb(port + LSR) & 0x20) != 0; }

void
serial_port::handle_interrupt()
{
    uint8_t iir = inb(m_port + IIR);

    while ((iir & 1) == 0) {
        uint8_t reg = 0;
        switch ((iir>>1) & 0x3) {
            case 0: // Modem status change
                reg = inb(m_port + MSR);
                handle_error(MSR, reg);
                break;

            case 1: // Transmit buffer empty
            case 2: // Received data available
                if (read_ready(m_port)) do_read();
                if (write_ready(m_port)) do_write();
                break;

            case 3: // Line status change
                reg = inb(m_port + LSR);
                handle_error(LSR, reg);
                break;
        }

        iir = inb(m_port + IIR);
    }
}

void
serial_port::do_read()
{
    size_t used = 0;
    uint8_t *data = nullptr;
    size_t avail = m_in_buffer.reserve(fifo_size, reinterpret_cast<void**>(&data));

    while (used < avail && read_ready(m_port)) {
        *data++ = inb(m_port);
        used++;
    }

    m_in_buffer.commit(used);
}

void
serial_port::do_write()
{
    // If another thread is writing data, just give up and
    // try again later
    util::scoped_lock lock {m_lock};

    uint8_t *data = nullptr;
    size_t n = m_out_buffer.get_block(reinterpret_cast<void**>(&data));

    m_writing = (n > 0);
    if (!m_writing) {
        m_out_buffer.consume(0);
        return;
    }

    if (n > fifo_size)
        n = fifo_size;

    for (size_t i = 0; i < n; ++i) {
        if (!write_ready(m_port)) {
            n = i;
            break;
        }
        outb(m_port, data[i]);
    }

    m_out_buffer.consume(n);
}

void
serial_port::handle_error(uint16_t reg, uint8_t value)
{
    while (1) asm ("hlt");
}

char
serial_port::read()
{
    uint8_t *data = nullptr;
    size_t n = m_in_buffer.get_block(reinterpret_cast<void**>(&data));
    if (!n) return 0;
    char c = *data;
    m_in_buffer.consume(1);
    return c;
}

size_t
serial_port::write(const char *c, size_t len)
{
    uint8_t *data = nullptr;
    size_t avail = m_out_buffer.reserve(len, reinterpret_cast<void**>(&data));

    memcpy(data, c, avail);
    m_out_buffer.commit(avail);

    if (!m_writing)
        do_write();

    return avail;
}

