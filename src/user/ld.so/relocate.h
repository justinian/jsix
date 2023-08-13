#pragma once
/// \file relocate.h
/// Image relocation services

#include <stdint.h>

void relocate_image(uintptr_t base, uintptr_t dyn_section);