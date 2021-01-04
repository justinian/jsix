#include "kutil/memory.h"
#include "kutil/no_construct.h"
#include "console.h"
#include "log.h"
#include "scheduler.h"

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

void
logger_task()
{
	uint8_t buffer[257];
	auto *ent = reinterpret_cast<log::logger::entry *>(buffer);
	auto *cons = console::get();

	log::info(logs::task, "Starting kernel logger task");
	g_logger.set_immediate(nullptr);

	scheduler &s = scheduler::get();

	while (true) {
		if(g_logger.get_entry(buffer, sizeof(buffer))) {
			buffer[ent->bytes] = 0;
			cons->set_color(level_colors[static_cast<int>(ent->severity)]);
			cons->printf("%7s %5s: %s\n",
				g_logger.area_name(ent->area),
				g_logger.level_name(ent->severity),
				ent->message);
			cons->set_color();
		} else {
			s.schedule();
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
