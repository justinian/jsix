#pragma once
/* Errors <errno.h>

   This file is part of the Public Domain C Library (PDCLib).
   Permission is granted to use, modify, and / or redistribute at will.
*/

#include "j6libc/int.h"

#define errno (*_PDCLIB_errno_func())

#define ERANGE _PDCLIB_ERANGE
#define EDOM _PDCLIB_EDOM
#define ENOMEM _PDCLIB_ENOMEM
#define EINVAL _PDCLIB_EINVAL
