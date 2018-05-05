#pragma once
#include <stdarg.h>
#include <stdint.h>

class console;


enum class logs
{
	boot,
	memory,
	apic,
	devices,

	max
};


class log
{
public:
	enum class level {debug, info, warn, error, fatal, max};

	static void init(console *cons);
	static void enable(logs type, level at_level);

	template <level L>
	static void trylog(logs area, const char *fmt, ...);
	using trylog_p = void (*)(logs area, const char *fmt, ...);

	static const trylog_p debug;
	static const trylog_p info;
	static const trylog_p warn;
	static const trylog_p error;
	static const trylog_p fatal;

private:
	void output(level severity, logs area, const char *fmt, va_list args);

	/// Bitmasks for what categories are enabled. fatal is
	/// always enabled, so leave it out.
	uint64_t m_enabled[static_cast<uint64_t>(level::max) - 1];
	console *m_cons;

	log();
	log(console *cons);
	static log s_log;
};
