#pragma once

#include "kernel_args.h"

namespace boot {
namespace loader {

kernel::entrypoint load_elf(const void *data, size_t size, uefi::boot_services *bs);

} // namespace loader
} // namespace boot
