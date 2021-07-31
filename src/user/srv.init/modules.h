#pragma once
/// \file modules.h
/// Routines for loading initial argument modules

namespace modules {

/// Load all modules
/// \arg address  The physical address of the first page of modules
void load_all(uintptr_t address);

} // namespace modules
