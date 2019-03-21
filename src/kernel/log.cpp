#include "kutil/constexpr_hash.h"
#include "kutil/memory.h"
#include "console.h"
#include "log.h"

namespace logs {

static uint8_t log_buffer[0x10000];
static log::logger g_logger(log_buffer, sizeof(log_buffer));

#define LOG(name, lvl) \
	log::area_t name = #name ## _h; \
	const char * name ## _name = #name;
#include "log_areas.inc"
#undef LOG

static const uint8_t level_colors[] = {0x07, 0x07, 0x0f, 0x0b, 0x09};

static void
output_log(log::area_t area, log::level severity, const char *message)
{
	auto *cons = console::get();
	cons->set_color(level_colors[static_cast<int>(severity)]);
	cons->printf("%9s %8s: %s\n",
			g_logger.area_name(area),
			g_logger.level_name(severity),
			message);
	cons->set_color();
}

void init()
{
	new (&g_logger) log::logger(log_buffer, sizeof(log_buffer));
	g_logger.set_immediate(output_log);

#define LOG(name, lvl) \
	g_logger.register_area(name, name ## _name, log::level ::lvl);
#include "log_areas.inc"
#undef LOG
}

} // namespace logs
