#include "j6/signals.h"
#include "kutil/memory.h"
#include "kutil/no_construct.h"
#include "console.h"
#include "log.h"
#include "objects/system.h"
#include "objects/thread.h"

static uint8_t log_buffer[0x10000];

// The logger is initialized _before_ global constructors are called,
// so that we can start log output immediately. Keep its constructor
// from being called here so as to not overwrite the previous initialization.
static kutil::no_construct<log::logger> __g_logger_storage;
log::logger &g_logger = __g_logger_storage.value;

static const uint8_t level_colors[] = {0x07, 0x07, 0x0f, 0x0b, 0x09};

static void
output_log(log::area_t area, log::level severity, const char *message)
{
    auto *cons = console::get();
    cons->set_color(level_colors[static_cast<int>(severity)]);
    cons->printf("%7s %5s: %s\n",
            g_logger.area_name(area),
            g_logger.level_name(severity),
            message);
    cons->set_color();
}

// For printf.c
extern "C" void putchar_(char c) {}

static void
log_flush()
{
    system &sys = system::get();
    sys.assert_signal(j6_signal_system_has_log);
}

void
logger_task()
{
    auto *cons = console::get();

    log::info(logs::task, "Starting kernel logger task");
    g_logger.set_immediate(nullptr);
    g_logger.set_flush(log_flush);

    thread &self = thread::current();
    system &sys = system::get();

    size_t buffer_size = 1;
    uint8_t *buffer = nullptr;

    while (true) {
        size_t size = g_logger.get_entry(buffer, buffer_size);
        if (size > buffer_size) {
            while (size > buffer_size) buffer_size *= 2;
            kutil::kfree(buffer);
            buffer = reinterpret_cast<uint8_t*>(kutil::kalloc(buffer_size));
            kassert(buffer, "Could not allocate logger task buffer");
            continue;
        }

        if(size) {
            auto *ent = reinterpret_cast<log::logger::entry *>(buffer);
            buffer[ent->bytes] = 0;

            cons->set_color(level_colors[static_cast<int>(ent->severity)]);
            cons->printf("%7s %5s: %s\n",
                g_logger.area_name(ent->area),
                g_logger.level_name(ent->severity),
                ent->message);
            cons->set_color();
        }

        if (!g_logger.has_log()) {
            sys.deassert_signal(j6_signal_system_has_log);
            sys.add_blocked_thread(&self);
            self.wait_on_signals(j6_signal_system_has_log);
        }
    }
}

void logger_init()
{
    new (&g_logger) log::logger(log_buffer, sizeof(log_buffer), output_log);
}

void logger_clear_immediate()
{
    g_logger.set_immediate(nullptr);
}
