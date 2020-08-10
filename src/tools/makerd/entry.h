#pragma once
#include <string>
#include <fstream>

class entry
{
public:
	entry(
		const std::string &in,
		const std::string &out,
		bool executable = false,
		bool symbols = false);

	inline const std::string & in() const { return m_in; }
	inline const std::string & out() const { return m_out; }
	inline const std::ifstream & file() const { return m_file; }

	inline bool executable() const { return m_exec; }
	inline bool symbols() const { return m_syms; }
	inline size_t size() const { return m_size; }
	inline bool good() const { return m_file.good(); }

private:
	std::string m_in;
	std::string m_out;
	std::ifstream m_file;
	size_t m_size;
	bool m_exec;
	bool m_syms;
};

