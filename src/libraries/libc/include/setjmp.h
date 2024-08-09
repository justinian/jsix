#pragma once
/** \file setjmp.h
  * Nonlocal jumps
  *
  * This file is part of the C standard library for the jsix operating
  * system.
  *
  * This Source Code Form is subject to the terms of the Mozilla Public
  * License, v. 2.0. If a copy of the MPL was not distributed with this
  * file, You can obtain one at https://mozilla.org/MPL/2.0/.
  */

#include <stdint.h>
#include <stdnoreturn.h>

typedef uint64_t jmp_buf[9];

int setjmp(jmp_buf env);
_Noreturn void longjmp(jmp_buf env, int val);
