#include <util/format.h>

#include "debugcon.h"
#include "interrupts.h"
#include "logger.h"

namespace debugcon {

namespace {
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

    inline void debug_newline() {
        static const char *newline = "\r\n";
        asm ( "rep outsb;" :: "c"(2), "d"(port), "S"(newline) );
    }
} // anon namespace

void
output(logs area, log::level level, const char *message, size_t mlen)
{
    char buffer [256];
    size_t dlen = util::format({buffer, sizeof(buffer)}, "%7s %7s| ",
            area_names[static_cast<uint8_t>(area)],
            level_names[static_cast<uint8_t>(level)]);
    debug_out(buffer, dlen);

    debug_out(message, mlen);
    debug_newline();
}

void
logger_task()
{
    using entry = log::logger::entry;

    uint64_t seen = 0;
    size_t buf_size = 128;
    uint8_t *buf = new uint8_t [buf_size];
    entry *header = reinterpret_cast<entry*>(buf);

    while (true) {
        size_t size = g_logger.get_entry(seen, buf, buf_size);
        if (size > buf_size) {
            delete [] buf;
            buf_size *= 2;
            buf = new uint8_t [buf_size];
            header = reinterpret_cast<entry*>(buf);
            continue;
        }

        output(header->area, header->severity, header->message, size - sizeof(entry));
        seen = header->id;
    }
}

} // namespace debugcon
