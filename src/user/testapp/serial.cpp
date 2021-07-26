#include "io.h"
#include "serial.h"


serial_port::serial_port() :
	m_port(0)
{
}

serial_port::serial_port(uint16_t port) :
	m_port(port)
{
	outb(port + 1, 0x00);  // Disable all interrupts
	outb(port + 3, 0x80);  // Enable the Divisor Latch Access Bit
	outb(port + 0, 0x01);  // Divisor low bit: 1 (115200 baud)
	outb(port + 1, 0x00);  // Divisor high bit
	outb(port + 3, 0x03);  // 8-N-1
	outb(port + 2, 0xe7);  // Clear and enable FIFO, enable 64 byte, 56 byte trigger
	outb(port + 4, 0x0b);  // Data terminal ready, Request to send, aux output 2 (irq enable)
	outb(port + 1, 0x03);  // Enable interrupts
}

bool serial_port::read_ready() { return (inb(m_port + 5) & 0x01) != 0; }

bool serial_port::write_ready() {
	uint8_t lsr = inb(m_port + 5);
	return (lsr & 0x20) != 0;
}

char
serial_port::read() {
	while (!read_ready());
	return inb(m_port);
}

void
serial_port::write(char c) {
	while (!write_ready());
	outb(m_port, c);
}

