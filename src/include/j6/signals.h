#pragma once
/// \file signals.h
/// Collection of constants for the j6_signal_t type

// Signals 0-15 are common to all types
#define j6_signal_no_handles        (1ull << 0)
#define j6_signal_closed            (1ull << 1)

// Signals 16-47 are defined per-object-type

// Event signals
#define j7_signal_event00           (1ull << 16)
#define j7_signal_event01           (1ull << 17)
#define j7_signal_event02           (1ull << 18)
#define j7_signal_event03           (1ull << 19)
#define j7_signal_event04           (1ull << 20)
#define j7_signal_event05           (1ull << 21)
#define j7_signal_event06           (1ull << 22)
#define j7_signal_event07           (1ull << 23)
#define j7_signal_event08           (1ull << 24)
#define j7_signal_event09           (1ull << 25)
#define j7_signal_event10           (1ull << 26)
#define j7_signal_event11           (1ull << 27)
#define j7_signal_event12           (1ull << 28)
#define j7_signal_event13           (1ull << 29)
#define j7_signal_event14           (1ull << 30)
#define j7_signal_event15           (1ull << 31)
#define j7_signal_event16           (1ull << 32)
#define j7_signal_event17           (1ull << 33)
#define j7_signal_event18           (1ull << 34)
#define j7_signal_event19           (1ull << 35)
#define j7_signal_event20           (1ull << 36)
#define j7_signal_event21           (1ull << 37)
#define j7_signal_event22           (1ull << 38)
#define j7_signal_event23           (1ull << 39)
#define j7_signal_event24           (1ull << 40)
#define j7_signal_event25           (1ull << 41)
#define j7_signal_event26           (1ull << 42)
#define j7_signal_event27           (1ull << 43)
#define j7_signal_event28           (1ull << 44)
#define j7_signal_event29           (1ull << 45)
#define j7_signal_event30           (1ull << 46)
#define j7_signal_event31           (1ull << 47)

// System signals
#define j6_signal_system_has_log    (1ull << 16)

// Channel signals
#define j6_signal_channel_can_send  (1ull << 16)
#define j6_signal_channel_can_recv  (1ull << 17)

// Endpoint signals
#define j6_signal_endpoint_can_send (1ull << 16)
#define j6_signal_endpoint_can_recv (1ull << 17)

// Signals 48-63 are user-defined signals
#define j6_signal_user0             (1ull << 48)
#define j6_signal_user1             (1ull << 49)
#define j6_signal_user2             (1ull << 50)
#define j6_signal_user3             (1ull << 51)
#define j6_signal_user4             (1ull << 52)
#define j6_signal_user5             (1ull << 53)
#define j6_signal_user6             (1ull << 54)
#define j6_signal_user7             (1ull << 55)
#define j6_signal_user8             (1ull << 56)
#define j6_signal_user9             (1ull << 57)
#define j6_signal_user10            (1ull << 58)
#define j6_signal_user11            (1ull << 59)
#define j6_signal_user12            (1ull << 60)
#define j6_signal_user13            (1ull << 61)
#define j6_signal_user14            (1ull << 62)
#define j6_signal_user15            (1ull << 63)

#define j6_signal_user_mask         (0xffffull << 48)
