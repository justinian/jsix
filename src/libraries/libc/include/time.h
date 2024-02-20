#pragma once
/** \file time.h
  * Date and time
  *
  * This file is part of the C standard library for the jsix operating
  * system.
  *
  * This Source Code Form is subject to the terms of the Mozilla Public
  * License, v. 2.0. If a copy of the MPL was not distributed with this
  * file, You can obtain one at https://mozilla.org/MPL/2.0/.
  */

#include <__j6libc/null.h>
#include <__j6libc/size_t.h>
#include <stdint.h>

#define CLOCKS_PER_SEC 1
#define TIME_UTC 0

typedef int64_t clock_t;
typedef int64_t time_t;

struct timespec
{
    time_t tv_sec;
    long tv_nsec;
};

struct tm
{
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;
    int tm_mon;
    int tm_year;
    int tm_wday;
    int tm_yday;
    int tm_isdst;
};

clock_t clock(void);
double difftime(time_t t1, time_t t0);
time_t mktime(struct tm *timeptr);
time_t time(time_t *timer);
int timespec_get(struct timespec *ts, int base);

char *asctime(const struct tm *timeptr);
char *ctime(const time_t *timer);
struct tm *gmtime(const time_t *timer);
struct tm *localtime(const time_t *timer);
size_t strftime(char *s, size_t maxsize, const char *fmt, const struct tm *timeptr);
