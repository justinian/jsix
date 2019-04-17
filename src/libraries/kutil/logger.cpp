#include "kutil/assert.h"
#include "kutil/constexpr_hash.h"
#include "kutil/logger.h"
#include "kutil/memory.h"
#include "kutil/printf.h"

namespace kutil {
namespace logs {
#define LOG(name, lvl) \
	const log::area_t name = #name ## _h; \
	const char * name ## _name = #name;
#include "log_areas.inc"
#undef LOG
}

namespace log {

using kutil::memset;
using kutil::memcpy;

logger *logger::s_log = nullptr;
const char *logger::s_level_names[] = {"", "debug", "info", "warn", "error", "fatal"};

logger::logger() :
	m_buffer(nullptr, 0),
	m_immediate(nullptr),
	m_sequence(0)
{
	memset(&m_levels, 0, sizeof(m_levels));
	memset(&m_names, 0, sizeof(m_names));
	s_log = this;
}

logger::logger(uint8_t *buffer, size_t size) :
	m_buffer(buffer, size),
	m_immediate(nullptr),
	m_sequence(0)
{
	memset(&m_levels, 0, sizeof(m_levels));
	memset(&m_names, 0, sizeof(m_names));
	s_log = this;

#define LOG(name, lvl) \
	register_area(logs::name, logs::name ## _name, log::level::lvl);
#include "log_areas.inc"
#undef LOG
}

void
logger::set_level(area_t area, level l)
{
	unsigned uarea = static_cast<unsigned>(area);
	uint8_t ulevel = static_cast<uint8_t>(l) & 0x0f;
	uint8_t &flags = m_levels[uarea / 2];
	if (uarea & 1)
		flags = (flags & 0x0f) | (ulevel << 4);
	else
		flags = (flags & 0xf0) | ulevel;
}

level
logger::get_level(area_t area)
{
	unsigned uarea = static_cast<unsigned>(area);
	uint8_t &flags = m_levels[uarea / 2];
	if (uarea & 1)
		return static_cast<level>((flags & 0xf0) >> 4);
	else
		return static_cast<level>(flags & 0x0f);
}

void
logger::register_area(area_t area, const char *name, level verbosity)
{
	m_names[area] = name;
	set_level(area, verbosity);
}

void
logger::output(level severity, area_t area, const char *fmt, va_list args)
{
	uint8_t buffer[256];
	entry *header = reinterpret_cast<entry *>(buffer);
	header->bytes = sizeof(entry);
	header->area = area;
	header->severity = severity;
	header->sequence = m_sequence++;

	header->bytes +=
		vsnprintf(header->message, sizeof(buffer) - sizeof(entry), fmt, args);

	if (m_immediate) {
		buffer[header->bytes] = 0;
		m_immediate(area, severity, header->message);
		return;
	}

	uint8_t *out;
	size_t n = m_buffer.reserve(header->bytes, reinterpret_cast<void**>(&out));
	if (n < sizeof(entry)) {
		m_buffer.commit(0); // Cannot even write the header, abort
		return;
	}

	if (n < header->bytes)
		header->bytes = n;

	memcpy(out, buffer, n);
	m_buffer.commit(n);
}

size_t
logger::get_entry(void *buffer, size_t size)
{
	void *out;
	size_t out_size = m_buffer.get_block(&out);
	entry *ent = reinterpret_cast<entry *>(out);
	if (out_size == 0 || out == 0)
		return 0;

	kassert(out_size >= sizeof(entry), "Couldn't read a full entry");
	if (out_size < sizeof(entry))
		return 0;

	kassert(size >= ent->bytes, "Didn't pass a big enough buffer");
	if (size < ent->bytes)
		return 0;

	memcpy(buffer, out, ent->bytes);
	m_buffer.consume(ent->bytes);
	return ent->bytes;
}

#define LOG_LEVEL_FUNCTION(name) \
	void name (area_t area, const char *fmt, ...) { \
		logger *l = logger::s_log; \
		if (!l) return; \
		level limit = l->get_level(area); \
		if (limit == level::none || level::name < limit) return; \
		va_list args; \
		va_start(args, fmt); \
		l->output(level::name, area, fmt, args); \
		va_end(args); \
	}

LOG_LEVEL_FUNCTION(debug);
LOG_LEVEL_FUNCTION(info);
LOG_LEVEL_FUNCTION(warn);
LOG_LEVEL_FUNCTION(error);

void fatal(area_t area, const char *fmt, ...)
{
	logger *l = logger::s_log;
	if (!l) return;

	va_list args;
	va_start(args, fmt);
	l->output(level::fatal, area, fmt, args);
	va_end(args);

	kassert(false, "log::fatal");
}

} // namespace log
} // namespace kutil
