#pragma once
/// \file address_manager.h
/// The virtual memory address space manager

#include "kutil/buddy_allocator.h"

namespace kutil {

using address_manager =
	buddy_allocator<
		12,  // Min allocation:  4KiB
		36   // Max allocation: 64GiB
	>;

} //namespace kutil
