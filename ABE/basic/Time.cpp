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

// File:    Time.cpp
// Author:  mtdcy.chen
// Changes: 
//          1. 20160701     initial version
//

#define LOG_TAG "Time" 
#include "Log.h"
#include "Time.h"

#include "Config.h" 

#include "compat/time.h"

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if defined(__MINGW32__)
#include "windows.h"
#endif

#ifdef __APPLE__
#include <mach/mach_time.h>
#endif

extern "C" {

    // NOTE: DO NOT print any log with Log here
    int64_t SystemTime() {
        struct timespec ts;
        relative_time(&ts);
        return ts.tv_sec * 1000000000LL + ts.tv_nsec;
    }

#if defined(__MINGW32__)
    int SleepTime(int64_t ns) {
        // XXX: not accurate.
        Sleep(ns/1000000);
        return 0;
    }
#else   // __MINGW32__ 
    int SleepTime(int64_t ns) {
#ifdef __APPLE__
        uint64_t now = mach_absolute_time();
        mach_timebase_info_data_t timebase;
        mach_timebase_info(&timebase);
        uint64_t to_wait = (ns * timebase.denom) / timebase.numer;
        return mach_wait_until(now + to_wait);
#else
        struct timespec ts;
        ts.tv_sec   = ns / 1000000000LL;
        ts.tv_nsec  = ns % 1000000000LL;
        return nanosleep(&ts, NULL);
        // man(3) usleep:
        // "The usleep() function is obsolescent. Use nanosleep(2) instead."
        //return usleep(usecs);
#endif
    }
#endif  // __MINGW32__ 
}
