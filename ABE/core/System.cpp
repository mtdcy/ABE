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

// File:    System.cpp
// Author:  mtdcy.chen
// Changes:
//          1. 20160701     initial version
//
#if defined(__APPLE__)
#define _DARWIN_C_SOURCE
#endif

#define LOG_TAG "System"
#include "Log.h"
#include "System.h"

#if defined(__ANDROID__)
// this is a code we copy from android
#include "private/cpu-features.h"
#elif defined(__APPLE__)
#include <sys/types.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#elif defined(_WIN32) || defined(__MINGW32__)
#include <sysinfoapi.h>
#else
#include <unistd.h>
#endif

#include <stdlib.h>

#include <time.h>

#if defined(_WIN32)
#include <Windows.h>
#endif

#include <errno.h>

BEGIN_DECLS

// https://stackoverflow.com/questions/150355/programmatically-find-the-number-of-cores-on-a-machine
UInt32 GetCpuCount() {
#if defined(__ANDROID__)
    return android_getCpuCount();
#elif defined(__APPLE__)
    int nm[2];
    UInt32 len = 4;
    UInt32 count;
    
    nm[0] = CTL_HW; nm[1] = HW_AVAILCPU;
    sysctl(nm, 2, &count, (size_t *)&len, Nil, 0);
    
    if(count < 1) {
        nm[1] = HW_NCPU;
        sysctl(nm, 2, &count, (size_t *)&len, Nil, 0);
        if(count < 1) { count = 1; }
    }
    return count;
#elif defined(_WIN32) || defined(__MINGW32__)
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    return sysinfo.dwNumberOfProcessors;
#else
    return sysconf(_SC_NPROCESSORS_ONLN);
#endif
}

const Char * GetEnvironmentValue(const Char *name) {
    static const Char * empty = "";
    const Char *value = getenv(name);
    return value ? value : empty;
}


END_DECLS

__BEGIN_NAMESPACE_ABE

// get system time in nsecs since Epoch. @see CLOCK_REALTIME
// @note it can jump forwards and backwards as the system time-of-day clock is changed, including by NTP.
static UInt64 SystemTimeEpoch() {
#if defined(_WIN32)
    // borrow from ffmpeg, 100ns resolution
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    Int64 t = (Int64)ft.dwHighDateTime << 32 | ft.dwLowDateTime;
    t -= 116444736000000000LL;    //1jan1601 to 1jan1970
    return t * 100LL;
#else
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_sec * 1000000000LL + ts.tv_nsec;
#endif
}

// get system time in nsecs since an arbitrary point, @see CLOCK_MONOTONIC
// For time measurement and timmer.
// @note It isn't affected by changes in the system time-of-day clock.
static UInt64 SystemTimeMonotonic() {
#if defined(_WIN32)
    static LARGE_INTEGER performanceFrequency = { 0 };
    if (ABE_UNLIKELY(performanceFrequency.QuadPart == 0)) {
        QueryPerformanceFrequency(&performanceFrequency);
    }
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    return (t.QuadPart * 1000000000LL) / performanceFrequency.QuadPart;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000LL + ts.tv_nsec;
#endif
}

Time Time::Now(Bool epoch) {
    Time time;
    time.mTime = epoch ? SystemTimeEpoch() : SystemTimeMonotonic();
    return time;
}

/**
 * suspend thread execution for an interval, @see man(2) nanosleep
 * @return return True on sleep complete, return False if was interrupted by signal
 * @note not all sleep implementation on different os will have guarantee.
 */
ABE_INLINE Bool Sleep(UInt64 ns, UInt64 *unslept) {
#if defined(_WIN32)
    // https://gist.github.com/Youka/4153f12cf2e17a77314c
    // FIXME: return the unslept time properly
    /* Windows sleep in 100ns units */
    /* Declarations */
    HANDLE timer;     /* Timer handle */
    LARGE_INTEGER li; /* Time defintion */
    if (unslept) *unslept = 0;
    /* Create timer */
    if (!(timer = CreateWaitableTimer(Nil, TRUE, Nil)))
        return False;
    
    /* Set timer properties */
    li.QuadPart = -ns;
    if (!SetWaitableTimer(timer, &li, 0, Nil, Nil, FALSE)) {
        CloseHandle(timer);
        return False;
    }
    /* Start & wait for timer */
    if (WaitForSingleObject(timer, INFINITE) != WAIT_OBJECT_0) {
        // failed ?
    }
    /* Clean resources */
    CloseHandle(timer);
    /* Slept without problems */
    return True;
#else
    struct timespec rqtp;
    struct timespec rmtp;
    rqtp.tv_sec     = ns / 1000000000LL;
    rqtp.tv_nsec    = ns % 1000000000LL;
    int rt = nanosleep(&rqtp, &rmtp);
    CHECK_TRUE(rt == 0 || errno == EINTR);
    if (rt == 0) return True;
    // return False and unslept time
    if (unslept) *unslept = rmtp.tv_sec * 1000000000LL + rmtp.tv_nsec;
    return False;
    // man(3) usleep:
    // "The usleep() function is obsolescent. Use nanosleep(2) instead."
    //return usleep(usecs);
#endif
}

Bool Timer::sleep(Time interval, Bool interrupt) {
    UInt64 ns = interval.nseconds();
    if (interrupt) return Sleep(ns, Nil);
    
    while (Sleep(ns, &ns) == False) { }
    return True;
}
__END_NAMESPACE_ABE
