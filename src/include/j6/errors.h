#pragma once
/// \file errors.h
/// Collection of constants for the j6_status_t type

#define j6_status_error             0x8000000000000000
#define j6_err(x)                   ((x) | j6_status_error)
#define j6_is_err(x)                (((x) & j6_status_error) == j6_status_error)

#define j6_status_ok                0x0000

#define j6_status_closed            0x1000
#define j6_status_destroyed         0x1001
#define j6_status_exists            0x1002

#define j6_err_nyi                  j6_err(0x0001)
#define j6_err_unexpected           j6_err(0x0002)
#define j6_err_invalid_arg          j6_err(0x0003)
#define j6_err_not_ready            j6_err(0x0004)
#define j6_err_insufficient         j6_err(0x0005)

