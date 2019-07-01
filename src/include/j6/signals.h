#pragma once
/// \file signals.h
/// Collection of constants for the j6_signal_t type

// Signals 0-7 are common to all types
#define j6_signal_no_handles		(1 << 0)

// Signals 8-15 are user-defined signals
#define j6_signal_user0				(1 << 8)
#define j6_signal_user1				(1 << 9)
#define j6_signal_user2				(1 << 10)
#define j6_signal_user3				(1 << 11)
#define j6_signal_user4				(1 << 12)
#define j6_signal_user5				(1 << 13)
#define j6_signal_user6				(1 << 14)
#define j6_signal_user7				(1 << 15)

// All other signals are type-specific
