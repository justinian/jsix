#include "serial.h"

namespace panicking {

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

inline void outb(uint16_t port, uint8_t val) {
    asm ( "outb %0, %1" :: "a"(val), "Nd"(port) );
}

inline uint8_t inb(uint16_t port) {
    uint8_t val;
    asm ( "inb %1, %0" : "=a"(val) : "Nd"(port) );
    return val;
}


serial_port::serial_port(uint16_t port) :
    m_port(port)
{
    outb(port + IER, 0x00);  // Disable all interrupts
    outb(port + LCR, 0x80);  // Enable the Divisor Latch Access Bit
    outb(port + DLL, 0x01);  // Divisor low byte: 1 (115200 baud)
    outb(port + DLH, 0x00);  // Divisor high byte
    outb(port + LCR, 0x03);  // 8-N-1, diable DLAB
    outb(port + FCR, 0xe7);  // Clear and enable FIFO, enable 64 byte, 56 byte trigger
    outb(port + MCR, 0x0b);  // Data terminal ready, Request to send, aux output 2 (irq enable)
}

inline bool read_ready(uint16_t port) { return (inb(port + LSR) & 0x01) != 0; }
inline bool write_ready(uint16_t port) { return (inb(port + LSR) & 0x20) != 0; }

void
serial_port::write(const char *s)
{
    char const *p = s;
    while (p && *p) {
        while (!write_ready(m_port)) outb(0x80, 0);
        outb(m_port, *p++);
    }
}

} // namespace panicking
