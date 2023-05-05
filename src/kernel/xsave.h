#pragma once
/// \file xsave.h 
/// XSAVE operations

#include <stddef.h>

extern const size_t &xsave_size;

void xsave_init();
void xsave_enable();
