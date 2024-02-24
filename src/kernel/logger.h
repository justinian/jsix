#pragma once
/// \file logger.h
/// Kernel logging facility.

#include <stdarg.h>
#include <stdint.h>

#include <util/counted.h>
#include <util/spinlock.h>

#include "objects/event.h"

enum class logs : uint8_t {
#define LOG(name, lvl) name,
#include <j6/tables/log_areas.inc>
#undef LOG
    COUNT
};

namespace log {

/// Size of the log ring buffer. Must be a power of two.
inline constexpr unsigned log_pages = 16;

enum class level : uint8_t {
    silent, fatal, error, warn, info, verbose, spam, max
};

constexpr unsigned areas_count =
    static_cast<unsigned>(logs::COUNT);

class logger
{
public:
    /// Default constructor. Creates a logger without a backing store.
    logger();

    /// Constructor. Logs are written to the given buffer.
    /// \arg buffer  Buffer to which logs are written
    logger(util::buffer buffer);

    /// Get the default logger.
    inline logger & get() { return *s_log; }

    /// Write to the log
    /// \arg severity  The severity of the message
    /// \arg area      The log area to write to
    /// \arg fmt       A printf-like format string
    inline void log(level severity, logs area, const char *fmt, ...)
    {
        level limit = get_level(area);
        if (severity > limit) return;

        va_list args;
        va_start(args, fmt);
        output(severity, area, fmt, args);
        va_end(args);
    }

    /// Get the next log entry from the buffer. Blocks the current thread until
    /// a log arrives if there are no entries newer than `seen`.
    /// \arg seen    The id of the last-seen log entry, or 0 for none
    /// \arg buffer  The buffer to copy the log message into
    /// \arg size    Size of the passed-in buffer, in bytes
    /// \returns     The size of the log entry (if larger than the
    ///              buffer, then no data was copied)
    size_t get_entry(uint64_t seen, void *buffer, size_t size);

    /// Check whether or not there's a new log entry to get
    /// \arg seen  The id of the last-seen log entry, or 0 for none
    inline bool has_entry(uint64_t seen) { return seen < m_count; }

private:
    friend void spam   (logs area, const char *fmt, ...);
    friend void verbose(logs area, const char *fmt, ...);
    friend void info   (logs area, const char *fmt, ...);
    friend void warn   (logs area, const char *fmt, ...);
    friend void error  (logs area, const char *fmt, ...);
    friend void fatal  (logs area, const char *fmt, ...);
    friend void log    (logs area, level severity, const char *fmt, ...);

    void output(level severity, logs area, const char *fmt, va_list args);

    inline void set_level(logs area, level l) {
        m_levels[static_cast<unsigned>(area)] = l;
    }

    inline level get_level(logs area) {
        return m_levels[static_cast<unsigned>(area)];
    }

    inline size_t offset(size_t i) const { return i & (m_buffer.count - 1); }

    inline size_t start() const { return offset(m_start); }
    inline size_t   end() const { return offset(m_end); }
    inline size_t  free() const { return m_buffer.count - (m_end - m_start); }

    level m_levels[areas_count];

    util::buffer m_buffer;
    size_t m_start, m_end;
    uint64_t m_count;

    wait_queue m_waiting;
    util::spinlock m_lock;

    static logger *s_log;
};

void spam   (logs area, const char *fmt, ...);
void verbose(logs area, const char *fmt, ...);
void info   (logs area, const char *fmt, ...);
void warn   (logs area, const char *fmt, ...);
void error  (logs area, const char *fmt, ...);
void fatal  (logs area, const char *fmt, ...);

void log    (logs area, level severity, const char *fmt, ...);

} // namespace log

extern log::logger &g_logger;
