#include <j6/memutils.h>
#include <util/format.h>
#include <util/no_construct.h>

#include "kassert.h"
#include "logger.h"

// The logger is initialized _before_ global constructors are called,
// so that we can start log output immediately. Keep its constructor
// from being called here so as to not overwrite the previous initialization.
static util::no_construct<log::logger> __g_logger_storage;
log::logger &g_logger = __g_logger_storage.value;


namespace log {

namespace {

} // anon namespace

logger *logger::s_log = nullptr;

logger::logger() :
    m_buffer {nullptr, 0},
    m_start {0},
    m_end {0},
    m_count {0}
{
    memset(&m_levels, 0, sizeof(m_levels));
    s_log = this;
}

logger::logger(util::buffer data) :
    m_buffer {data},
    m_start {0},
    m_end {0},
    m_count {0}
{
    kassert((data.count & (data.count - 1)) == 0,
        "log buffer size must be a power of two");

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
    static constexpr size_t buffer_len = 256;
    static constexpr size_t message_len = buffer_len - sizeof(j6_log_entry);

    char buffer[buffer_len];
    j6_log_entry *header = reinterpret_cast<j6_log_entry *>(buffer);

    size_t size = sizeof(j6_log_entry);
    size += util::vformat({header->message, message_len}, fmt, args);

    util::scoped_lock lock {m_lock};

    while (free() < size) {
        // Remove old entries until there's enough space
        const j6_log_entry *first = util::at<const j6_log_entry>(m_buffer, start());
        m_start += first->bytes;
    }

    header->id = ++m_count;
    header->bytes = size;
    header->severity = static_cast<uint8_t>(severity);
    header->area = static_cast<uint8_t>(area);

    memcpy(util::at<void>(m_buffer, end()), buffer, size);
    m_end += size;

    m_waiting.clear();
}

size_t
logger::get_entry(uint64_t seen, void *buffer, size_t size)
{
    util::scoped_lock lock {m_lock};

    while (seen == m_count) {
        lock.release();
        m_waiting.wait();
        lock.reacquire();
    }

    size_t off = m_start;
    j6_log_entry *ent = util::at<j6_log_entry>(m_buffer, offset(off));
    while (seen >= ent->id) {
        off += ent->bytes;
        kassert(off < m_end, "Got to the end while looking for new log entry");
        ent = util::at<j6_log_entry>(m_buffer, offset(off));
    }

    if (size >= ent->bytes)
        memcpy(buffer, ent, ent->bytes);

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

void
fatal(logs area, const char *fmt, ...)
{
    logger *l = logger::s_log;
    if (!l) return;

    va_list args;
    va_start(args, fmt);
    l->output(level::fatal, area, fmt, args);
    va_end(args);

    kassert(false, "log::fatal");
}

void
log(logs area, level severity, const char *fmt, ...)
{
    logger *l = logger::s_log;
    if (!l) return;

    level limit = l->get_level(area);
    if (severity > limit) return;

    va_list args;
    va_start(args, fmt);
    l->output(severity, area, fmt, args);
    va_end(args);
}

} // namespace log
