#pragma once
/// \file debugcon.h
/// Kernel debugcon log output process

#include "scheduler.h"

namespace debugcon {

static constexpr bool enable =
#ifdef __jsix_config_debug
    true;
#else
    false;
#endif

static constexpr uint16_t port = 0x6600;

void logger_task();
void write(const char *fmt, ...);

inline void
init_logger()
{
    if constexpr (enable) {
        static constexpr uint8_t pri = scheduler::max_priority - 1;
        scheduler &s = scheduler::get();
        s.create_kernel_task(logger_task, pri, true);
    }
}

} // namespace debugcon
