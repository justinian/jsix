#pragma once
/// \file logger.h
/// Kernel logging facility.

#include <stdarg.h>
#include <stdint.h>

#include <util/bip_buffer.h>
#include <util/spinlock.h>

#include "objects/event.h"

enum class logs : uint8_t {
#define LOG(name, lvl) name,
#include <j6/tables/log_areas.inc>
#undef LOG
    COUNT
};

namespace log {

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
    /// \arg size    Size of `buffer`, in bytes
    logger(uint8_t *buffer, size_t size);

    /// Get the default logger.
    inline logger & get() { return *s_log; }

    /// Get the registered name for a given area
    inline const char * area_name(logs area) const { return s_area_names[static_cast<unsigned>(area)]; }

    /// Get the name of a level
    inline const char * level_name(level l) const { return s_level_names[static_cast<unsigned>(l)]; }

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

    struct entry
    {
        uint8_t bytes;
        logs area;
        level severity;
        char message[0];
    };

    /// Get the next log entry from the buffer
    /// \arg buffer  The buffer to copy the log message into
    /// \arg size    Size of the passed-in buffer, in bytes
    /// \returns     The size of the log entry (if larger than the
    ///              buffer, then no data was copied)
    size_t get_entry(void *buffer, size_t size);

    /// Get whether there is currently data in the log buffer
    inline bool has_log() const { return m_buffer.size(); }

private:
    friend void spam   (logs area, const char *fmt, ...);
    friend void verbose(logs area, const char *fmt, ...);
    friend void info   (logs area, const char *fmt, ...);
    friend void warn   (logs area, const char *fmt, ...);
    friend void error  (logs area, const char *fmt, ...);
    friend void fatal  (logs area, const char *fmt, ...);

    void output(level severity, logs area, const char *fmt, va_list args);

    inline void set_level(logs area, level l) {
        m_levels[static_cast<unsigned>(area)] = l;
    }

    inline level get_level(logs area) {
        return m_levels[static_cast<unsigned>(area)];
    }

    obj::event m_event;

    level m_levels[areas_count];

    util::bip_buffer m_buffer;
    util::spinlock m_lock;

    static logger *s_log;
    static const char *s_area_names[areas_count+1];
    static const char *s_level_names[static_cast<unsigned>(level::max)];
};

void spam   (logs area, const char *fmt, ...);
void verbose(logs area, const char *fmt, ...);
void info   (logs area, const char *fmt, ...);
void warn   (logs area, const char *fmt, ...);
void error  (logs area, const char *fmt, ...);
void fatal  (logs area, const char *fmt, ...);

extern log::logger &g_logger;

} // namespace log

void logger_init();
