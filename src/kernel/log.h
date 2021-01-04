#pragma once

#include "kutil/logger.h"

namespace log = kutil::log;
namespace logs = kutil::logs;

void logger_init();
void logger_clear_immediate();
void logger_task();
