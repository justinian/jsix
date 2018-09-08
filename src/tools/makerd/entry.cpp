#include "entry.h"

entry::entry(const std::string &in, const std::string &out, bool executable) :
	m_in(in),
	m_out(out),
	m_file(in, std::ios_base::binary),
	m_exec(executable)
{
	m_file.seekg(0, std::ios_base::end);
	m_size = m_file.tellg();
	m_file.seekg(0);
}

