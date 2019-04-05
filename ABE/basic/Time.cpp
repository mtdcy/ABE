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

#if defined(__APPLE__)
#include "basic/compat/time_macos.h"
#else
#endif

#define LOG_TAG "Time"
#include "Log.h"
#include "Time.h"

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if defined(__MINGW32__)
#include "windows.h"
#endif

__BEGIN_DECLS

int64_t SystemTimeNs() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

__ABE_INLINE bool _Sleep(int64_t ns, int64_t *unslept) {
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
}

bool SleepForInterval(int64_t ns) {
    return _Sleep(ns, NULL);
}

void SleepTimeNs(int64_t ns) {
    while (_Sleep(ns, &ns) == false) { }
}

__END_DECLS

