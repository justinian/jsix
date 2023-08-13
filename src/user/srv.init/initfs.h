#pragma once
/// \file initfs.h
/// Temporary VFS service until we can load the real VFS service

#include <j6/types.h>

namespace j6romfs { class fs; }

void initfs_start(j6romfs::fs &fs, j6_handle_t mb);
void initfs_stop();