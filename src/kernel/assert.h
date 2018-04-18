#pragma once

[[noreturn]] void __kernel_assert(const char *file, unsigned line, const char *message);

#define kassert(stmt, message)  if(!(stmt)) { __kernel_assert(__FILE__, __LINE__, (message)); } else {}
