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

// File:    Time.h
// Author:  mtdcy.chen
// Changes: 
//          1. 20160701     initial version
//

#ifndef ABE_HEADERS_TIME_H
#define ABE_HEADERS_TIME_H

#include <ABE/core/Types.h>

__BEGIN_DECLS

// get system time in nsecs since Epoch. @see CLOCK_REALTIME
// @note it can jump forwards and backwards as the system time-of-day clock is changed, including by NTP.
ABE_EXPORT int64_t SystemTimeEpoch();

// get system time in nsecs since an arbitrary point, @see CLOCK_MONOTONIC
// For time measurement and timmer.
// @note It isn't affected by changes in the system time-of-day clock.
ABE_EXPORT int64_t SystemTimeMonotonic();

#define SystemTimeNs()      SystemTimeMonotonic()
#define SystemTimeUs()      (SystemTimeMonotonic() / 1000LL)
#define SystemTimeMs()      (SystemTimeMonotonic() / 1000000LL)

/**
 * suspend thread execution for an interval, @see man(2) nanosleep
 * @return return true on sleep complete, return false if was interrupted by signal
 * @note not all sleep implementation on different os will have guarantee.
 */
ABE_EXPORT bool SleepForInterval(int64_t ns);

/**
 * suspend thread execution for an interval, guarantee time elapsed
 */
ABE_EXPORT void SleepForIntervalWithoutInterrupt(int64_t ns);
#define SleepTimeNs(ns)     SleepForIntervalWithoutInterrupt(ns)
#define SleepTimeUs(us)     SleepForIntervalWithoutInterrupt(us * 1000LL)
#define SleepTimeMs(ms)     SleepForIntervalWithoutInterrupt(ms * 1000000LL)

__END_DECLS

#endif // ABE_HEADERS_TIME_H
