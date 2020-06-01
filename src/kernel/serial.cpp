#include "io.h"
#include "serial.h"

serial_port g_com1(COM1);


serial_port::serial_port() :
	m_port(0)
{
}

serial_port::serial_port(uint16_t port) :
	m_port(port)
{
}

bool serial_port::read_ready() { return (inb(m_port + 5) & 0x01) != 0; }
bool serial_port::write_ready() { return (inb(m_port + 5) & 0x20) != 0; }

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

