#pragma once
/// \file signals.h
/// Collection of constants for the j6_signal_t type

// Signals 0-15 are common to all types
#define j6_signal_no_handles		(1ull << 0)

// Signals 16-47 are defined per-object-type

// Process signals
#define j6_signal_process_exit		(1ull << 16)

// Thread signals
#define j6_signal_thread_exit		(1ull << 16)

// Channel signals
#define j6_signal_channel_closed	(1ull << 16)
#define j6_signal_channel_can_send	(1ull << 17)
#define j6_signal_channel_can_recv	(1ull << 18)

// Signals 48-63 are user-defined signals
#define j6_signal_user0				(1ull << 48)
#define j6_signal_user1				(1ull << 49)
#define j6_signal_user2				(1ull << 50)
#define j6_signal_user3				(1ull << 51)
#define j6_signal_user4				(1ull << 52)
#define j6_signal_user5				(1ull << 53)
#define j6_signal_user6				(1ull << 54)
#define j6_signal_user7				(1ull << 55)
#define j6_signal_user8				(1ull << 56)
#define j6_signal_user9				(1ull << 57)
#define j6_signal_user10			(1ull << 58)
#define j6_signal_user11			(1ull << 59)
#define j6_signal_user12			(1ull << 60)
#define j6_signal_user13			(1ull << 61)
#define j6_signal_user14			(1ull << 62)
#define j6_signal_user15			(1ull << 63)

#define j6_signal_user_mask			(0xffffull << 48)
