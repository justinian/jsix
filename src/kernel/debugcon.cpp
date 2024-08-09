#include <util/format.h>

#include "debugcon.h"
#include "interrupts.h"
#include "logger.h"

namespace debugcon {

namespace {
    static const uint8_t level_colors[] = {0x00, 0x09, 0x01, 0x0b, 0x0f, 0x07, 0x08};
    const char *level_names[] = {"", "fatal", "error", "warn", "info", "verbose", "spam"};
    const char *area_names[] = {
#define LOG(name, lvl) #name ,
#include <j6/tables/log_areas.inc>
#undef LOG
        nullptr
    };

    inline void debug_out(const char *msg, size_t size) {
        asm ( "rep outsb;" :: "c"(size), "d"(port), "S"(msg) );
    }

    inline void debug_endline() {
        static const char *newline = "\e[0m\r\n";
        asm ( "rep outsb;" :: "c"(6), "d"(port), "S"(newline) );
    }
} // anon namespace

void
write(const char *fmt, ...)
{
    char buffer[256];

    va_list va;
    va_start(va, fmt);
    size_t n = util::vformat({buffer, sizeof(buffer)}, fmt, va);
    va_end(va);

    debug_out(buffer, n);
    debug_out("\r\n", 2);
}

void
output(j6_log_entry *entry)
{
    char buffer [256];
    size_t dlen = util::format({buffer, sizeof(buffer)}, "\e[38;5;%dm%7s %7s\e[38;5;14m\u2502\e[38;5;%dm ",
            level_colors[entry->severity],
            area_names[entry->area],
            level_names[entry->severity],
            level_colors[entry->severity]);

    debug_out(buffer, dlen);

    debug_out(entry->message, entry->bytes - sizeof(j6_log_entry));
    debug_endline();
}

void
logger_task()
{
    using entry = j6_log_entry;

    uint64_t seen = 0;
    size_t buf_size = 128;
    uint8_t *buf = new uint8_t [buf_size];

    while (true) {
        size_t size = g_logger.get_entry(seen, buf, buf_size);
        if (size > buf_size) {
            delete [] buf;
            buf_size *= 2;
            buf = new uint8_t [buf_size];
            continue;
        }

        entry *header = reinterpret_cast<entry*>(buf);
        while (size > sizeof(entry)) {
            output(header);
            seen = header->id;
            size -= header->bytes;
            header = util::offset_pointer(header, header->bytes);
        }
    }
}

} // namespace debugcon
