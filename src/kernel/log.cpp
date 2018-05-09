#include <type_traits>
#include "kutil/assert.h"
#include "kutil/memory.h"
#include "console.h"
#include "log.h"


static const uint64_t default_enabled[] = {0x00, 0xff, 0xff, 0xff};
static const uint8_t level_colors[] = {0x07, 0x0f, 0x0b, 0x09};

static const char *levels[] = {"debug", " info", " warn", "error", "fatal"};
static const char *areas[] = {
	"boot",
	"mem ",
	"apic",
	"dev ",
	"driv",

	nullptr
};


log log::s_log;

log::log() : m_cons(nullptr)
{
	kassert(0, "Invalid log constructor");
}

log::log(console *cons) :
	m_cons(cons)
{
	const int num_levels = static_cast<int>(level::max) - 1;
	for (int i = 0; i < num_levels; ++i)
		m_enabled[i] = default_enabled[i];
}

void
log::init(console *cons)
{
	new (&s_log) log(cons);
	log::info(logs::boot, "Logging system initialized.");
}

static inline uint64_t
bit_mask(logs area) { return 1 << static_cast<uint64_t>(area); }

void
log::enable(logs type, level at_level)
{
	using under_t = std::underlying_type<level>::type;
	under_t at = static_cast<under_t>(at_level);
	under_t max = sizeof(m_enabled) / sizeof(m_enabled[0]);

	for (under_t i = 0; i < max; ++i) {
		if (i >= at)
			s_log.m_enabled[i] |= bit_mask(type);
		else
			s_log.m_enabled[i] &= ~bit_mask(type);
	}
}

template <>
void
log::trylog<log::level::fatal>(logs area, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	s_log.output(level::fatal, area, fmt, args);
	va_end(args);
	while(1) __asm__ ("hlt");
}

template <log::level L>
void
log::trylog(logs area, const char *fmt, ...)
{
	auto i = static_cast<std::underlying_type<level>::type>(L);
	if ((s_log.m_enabled[i] & bit_mask(area)) == 0) return;

	va_list args;
	va_start(args, fmt);
	s_log.output(L, area, fmt, args);
	va_end(args);
}

void
log::output(level severity, logs area, const char *fmt, va_list args)
{
	m_cons->set_color(level_colors[static_cast<int>(severity)]);

	m_cons->printf("%s %s: ",
			areas[static_cast<int>(area)],
			levels[static_cast<int>(severity)]);
	m_cons->vprintf(fmt, args);
	m_cons->set_color();
	m_cons->puts("\n");
}

const log::trylog_p log::debug = &trylog<level::debug>;
const log::trylog_p log::info = &trylog<level::info>;
const log::trylog_p log::warn = &trylog<level::warn>;
const log::trylog_p log::error = &trylog<level::error>;
const log::trylog_p log::fatal = &trylog<level::fatal>;
