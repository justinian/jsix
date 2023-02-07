#include <new>
#include <string.h>

#include <util/format.h>
#include <util/no_construct.h>

#include "assert.h"
#include "logger.h"
#include "memory.h"
#include "objects/system.h"
#include "objects/thread.h"

static constexpr bool j6_debugcon_enable = false;
static constexpr uint16_t j6_debugcon_port = 0x6600;

static uint8_t log_buffer[log::logger::log_pages * arch::frame_size];

// The logger is initialized _before_ global constructors are called,
// so that we can start log output immediately. Keep its constructor
// from being called here so as to not overwrite the previous initialization.
static util::no_construct<log::logger> __g_logger_storage;
log::logger &g_logger = __g_logger_storage.value;


namespace log {

logger *logger::s_log = nullptr;
const char *logger::s_level_names[] = {"", "fatal", "error", "warn", "info", "verbose", "spam"};
const char *logger::s_area_names[] = {
#define LOG(name, lvl) #name ,
#include <j6/tables/log_areas.inc>
#undef LOG
    nullptr
};

inline void
debug_out(const char *msg, size_t size)
{
    asm ( "rep outsb;" :: "c"(size), "d"(j6_debugcon_port), "S"(msg) );
}

inline void
debug_newline()
{
    static const char *newline = "\r\n";
    asm ( "rep outsb;" :: "c"(2), "d"(j6_debugcon_port), "S"(newline) );
}

logger::logger() :
    m_buffer(nullptr, 0)
{
    memset(&m_levels, 0, sizeof(m_levels));
    s_log = this;
}

logger::logger(uint8_t *buffer, size_t size) :
    m_buffer(buffer, size)
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
    char buffer[256];

    if constexpr (j6_debugcon_enable) {
        size_t dlen = util::format({buffer, sizeof(buffer)}, "%7s %7s| ",
                s_area_names[static_cast<uint8_t>(area)],
                s_level_names[static_cast<uint8_t>(severity)]);
        debug_out(buffer, dlen);
    }

    entry *header = reinterpret_cast<entry *>(buffer);
    header->bytes = sizeof(entry);
    header->area = area;
    header->severity = severity;

    size_t mlen = util::vformat({header->message, sizeof(buffer) - sizeof(entry) - 1}, fmt, args);
    header->message[mlen] = 0;
    header->bytes += mlen + 1;

    if constexpr (j6_debugcon_enable) {
        debug_out(header->message, mlen);
        debug_newline();
    }

    util::scoped_lock lock {m_lock};

    uint8_t *out;
    size_t n = m_buffer.reserve(header->bytes, reinterpret_cast<void**>(&out));
    if (n < header->bytes) {
        m_buffer.commit(0); // Cannot write the message, give up
        return;
    }

    memcpy(out, buffer, n);
    m_buffer.commit(n);

    m_event.signal(1);
}

size_t
logger::get_entry(void *buffer, size_t size)
{
    util::scoped_lock lock {m_lock};

    void *out;
    size_t out_size = m_buffer.get_block(&out);
    if (out_size == 0 || out == 0) {
        lock.release();
        m_event.wait();
        lock.reacquire();
        out_size = m_buffer.get_block(&out);

        if (out_size == 0 || out == 0)
            return 0;
    }

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
        if (level::name > limit) return; \
        va_list args; \
        va_start(args, fmt); \
        l->output(level::name, area, fmt, args); \
        va_end(args); \
    }

LOG_LEVEL_FUNCTION(spam);
LOG_LEVEL_FUNCTION(verbose);
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


void logger_init()
{
    new (&g_logger) log::logger(log_buffer, sizeof(log_buffer));
}
