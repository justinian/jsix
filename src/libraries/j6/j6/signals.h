#pragma once
/// \file signals.h
/// Collection of constants for the j6_signal_t type

// Signals 56-63 are common to all types
#define j6_signal_no_handles        (1ull << 56)
#define j6_signal_closed            (1ull << 57)
#define j6_signal_global_mask       0xff00000000000000

// Signals 0-55 are defined per object type

// System signals
#define j6_signal_system_has_log    (1ull <<  0)

// Channel signals
#define j6_signal_channel_can_send  (1ull <<  0)
#define j6_signal_channel_can_recv  (1ull <<  1)

// Endpoint signals
#define j6_signal_endpoint_can_send (1ull <<  0)
#define j6_signal_endpoint_can_recv (1ull <<  1)

