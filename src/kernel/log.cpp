#include "kutil/constexpr_hash.h"
#include "kutil/memory.h"
#include "log.h"

namespace logs {

static uint8_t log_buffer[0x10000];
static log::logger g_logger(log_buffer, sizeof(log_buffer));

#define LOG(name, lvl) \
	log::area_t name = #name ## _h; \
	const char * name ## _name = #name;
#include "log_areas.inc"
#undef LOG

void init()
{
	new (&g_logger) log::logger(log_buffer, sizeof(log_buffer));

#define LOG(name, lvl) \
	g_logger.register_area(name, name ## _name, log::level ::lvl);
#include "log_areas.inc"
#undef LOG
}

} // namespace logs
