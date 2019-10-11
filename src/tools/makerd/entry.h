#pragma once
#include <string>
#include <fstream>
#include "initrd/headers.h"

class entry
{
public:
	entry(const std::string &in, const std::string &out, initrd::file_type type);

	inline const std::string & in() const { return m_in; }
	inline const std::string & out() const { return m_out; }
	inline const std::ifstream & file() const { return m_file; }

	inline initrd::file_type type() const { return m_type; }
	inline size_t size() const { return m_size; }
	inline bool good() const { return m_file.good(); }

private:
	std::string m_in;
	std::string m_out;
	std::ifstream m_file;
	size_t m_size;
	initrd::file_type m_type;
};

