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

__BEGIN_DECLS

// https://stackoverflow.com/questions/150355/programmatically-find-the-number-of-cores-on-a-machine
size_t GetCpuCount() {
#if defined(__ANDROID__)
    return android_getCpuCount();
#elif defined(__APPLE__)
    int nm[2];
    size_t len = 4;
    uint32_t count;
    
    nm[0] = CTL_HW; nm[1] = HW_AVAILCPU;
    sysctl(nm, 2, &count, &len, NULL, 0);
    
    if(count < 1) {
        nm[1] = HW_NCPU;
        sysctl(nm, 2, &count, &len, NULL, 0);
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

const char * GetEnvironmentValue(const char *name) {
    static const char * empty = "";
    const char *value = getenv(name);
    return value ? value : empty;
}

int64_t SystemTimeEpoch() {
#if defined(_WIN32)
    // borrow from ffmpeg, 100ns resolution
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    int64_t t = (int64_t)ft.dwHighDateTime << 32 | ft.dwLowDateTime;
    t -= 116444736000000000LL;    //1jan1601 to 1jan1970
    return t * 100LL;
#else
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_sec * 1000000000LL + ts.tv_nsec;
#endif
}

int64_t SystemTimeMonotonic() {
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

ABE_INLINE bool _Sleep(int64_t ns, int64_t *unslept) {
#if defined(_WIN32)
    // https://gist.github.com/Youka/4153f12cf2e17a77314c
    // FIXME: return the unslept time properly
    /* Windows sleep in 100ns units */
    /* Declarations */
    HANDLE timer;     /* Timer handle */
    LARGE_INTEGER li; /* Time defintion */
    if (unslept) *unslept = 0;
    /* Create timer */
    if (!(timer = CreateWaitableTimer(NULL, TRUE, NULL)))
        return false;
    
    /* Set timer properties */
    li.QuadPart = -ns;
    if (!SetWaitableTimer(timer, &li, 0, NULL, NULL, FALSE)) {
        CloseHandle(timer);
        return false;
    }
    /* Start & wait for timer */
    if (WaitForSingleObject(timer, INFINITE) != WAIT_OBJECT_0) {
        // failed ?
    }
    /* Clean resources */
    CloseHandle(timer);
    /* Slept without problems */
    return true;
#else
    struct timespec rqtp;
    struct timespec rmtp;
    rqtp.tv_sec     = ns / 1000000000LL;
    rqtp.tv_nsec    = ns % 1000000000LL;
    int rt = nanosleep(&rqtp, &rmtp);
    CHECK_TRUE(rt == 0 || errno == EINTR);
    if (rt == 0) return true;
    // return false and unslept time
    if (unslept) *unslept = rmtp.tv_sec * 1000000000LL + rmtp.tv_nsec;
    return false;
    // man(3) usleep:
    // "The usleep() function is obsolescent. Use nanosleep(2) instead."
    //return usleep(usecs);
#endif
}

bool SleepForInterval(int64_t ns) {
    return _Sleep(ns, NULL);
}

void SleepForIntervalWithoutInterrupt(int64_t ns) {
    while (_Sleep(ns, &ns) == false) { }
}

__END_DECLS
