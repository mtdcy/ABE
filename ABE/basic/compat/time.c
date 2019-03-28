/******************************************************************************
 * Copyright (c) 2016, Chen Fang <mtdcy.chen@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, 
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation 
 *    and/or other materials provided with the distribution.
 * 
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 *  POSSIBILITY OF SUCH DAMAGE.
 ******************************************************************************/


// File:    time.c
// Author:  mtdcy.chen
// Changes: 
//          1. 20161012     initial version
//

#ifdef __linux__
#define _BSD_SOURCE     // for usleep
#endif 

#include "Config.h"
#include "basic/compat/time.h"

#include <errno.h>
#include <stdint.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#if defined(__MINGW32__)
#include "windows.h"
#endif

#ifdef __APPLE__
#include <mach/mach_time.h>
#endif

#ifdef __linux__
#define __need_timespec 
#endif
#include <time.h>
#include <sys/time.h>

// NOTE: DO NOT print any log with Log here


// http://stackoverflow.com/questions/5167269/clock-gettime-alternative-in-mac-os-x

void relative_time(struct timespec *ts) {
#if defined(__APPLE__)
    // https://developer.apple.com/library/content/qa/qa1398/_index.html
    mach_timebase_info_data_t timebase;
    uint64_t ns = mach_absolute_time();
    mach_timebase_info(&timebase);
    ns = (ns * timebase.numer) / timebase.denom;
    ts->tv_sec  = ns / 1000000000LL;
    ts->tv_nsec = ns % 1000000000LL;
#elif defined(HAVE_CLOCK_GETTIME) && defined(CLOCK_MONOTONIC)
    clock_gettime(CLOCK_MONOTONIC, ts);
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    ts->tv_sec  = tv.tv_sec;
    ts->tv_nsec = tv.tv_usec * 1000LL;
#endif
}

void absolute_time(struct timespec *ts) {
#if defined(HAVE_CLOCK_GETTIME) && defined(CLOCK_REALTIME)
    clock_gettime(CLOCK_REALTIME, ts);
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    ts->tv_sec  = tv.tv_sec;
    ts->tv_nsec = tv.tv_usec * 1000LL;
#endif
}

void absolute_time_later(struct timespec *ts, uint64_t nsecs) {
    absolute_time(ts);
    ts->tv_nsec += nsecs;
    if (ts->tv_nsec >= 1000000000LL) {
        ts->tv_sec += ts->tv_nsec / 1000000000LL;
        ts->tv_nsec = ts->tv_nsec % 1000000000LL;
    }
}
