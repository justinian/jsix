#include <string.h>

#include <util/constexpr_hash.h>

#include "assert.h"
#include "logger.h"
#include "objects/system.h"
#include "printf/printf.h"

namespace log {

logger *logger::s_log = nullptr;
const char *logger::s_level_names[] = {"", "debug", "info", "warn", "error", "fatal"};
const char *logger::s_area_names[] = {
#define LOG(name, lvl) #name ,
#include <j6/tables/log_areas.inc>
#undef LOG
    nullptr
};

logger::logger(logger::immediate_cb output) :
    m_buffer(nullptr, 0),
    m_immediate(output),
    m_sequence(0)
{
    memset(&m_levels, 0, sizeof(m_levels));
    s_log = this;
}

logger::logger(uint8_t *buffer, size_t size, logger::immediate_cb output) :
    m_buffer(buffer, size),
    m_immediate(output),
    m_sequence(0)
{
    memset(&m_levels, 0, sizeof(m_levels));
    s_log = this;

#define LOG(name, lvl) \
    set_level(logs::name, log::level::lvl);
#include <j6/tables/log_areas.inc>
#undef LOG
}

void
logger::output(level severity, logs area, const char *fmt, va_list args)
{
    uint8_t buffer[256];
    entry *header = reinterpret_cast<entry *>(buffer);
    header->bytes = sizeof(entry);
    header->area = area;
    header->severity = severity;
    header->sequence = m_sequence++;

    size_t mlen = vsnprintf(header->message, sizeof(buffer) - sizeof(entry) - 1, fmt, args);
    header->message[mlen] = 0;
    header->bytes += mlen + 1;

    util::scoped_lock lock {m_lock};

    if (m_immediate) {
        buffer[header->bytes] = 0;
        m_immediate(area, severity, header->message);
        return;
    }

    uint8_t *out;
    size_t n = m_buffer.reserve(header->bytes, reinterpret_cast<void**>(&out));
    if (n < header->bytes) {
        m_buffer.commit(0); // Cannot write the message, give up
        return;
    }

    memcpy(out, buffer, n);
    m_buffer.commit(n);

    system &sys = system::get();
    sys.assert_signal(j6_signal_system_has_log);
}

size_t
logger::get_entry(void *buffer, size_t size)
{
    util::scoped_lock lock {m_lock};

    void *out;
    size_t out_size = m_buffer.get_block(&out);
    if (out_size == 0 || out == 0)
        return 0;

    kassert(out_size >= sizeof(entry), "Couldn't read a full entry");
    if (out_size < sizeof(entry))
        return 0;

    entry *ent = reinterpret_cast<entry *>(out);
    if (size >= ent->bytes) {
        memcpy(buffer, out, ent->bytes);
        m_buffer.consume(ent->bytes);
    }

    return ent->bytes;
}

#define LOG_LEVEL_FUNCTION(name) \
    void name (logs area, const char *fmt, ...) { \
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

void fatal(logs area, const char *fmt, ...)
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
