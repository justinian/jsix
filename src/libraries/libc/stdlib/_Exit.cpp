/** \file _Exit.cpp
  *
  * This file is part of the C standard library for the jsix operating
  * system.
  *
  * This Source Code Form is subject to the terms of the Mozilla Public
  * License, v. 2.0. If a copy of the MPL was not distributed with this
  * file, You can obtain one at https://mozilla.org/MPL/2.0/.
  */

#include <stdlib.h>
#include <j6/syscalls.h>

void
_Exit( int status )
{
    j6_process_exit(status);
}
