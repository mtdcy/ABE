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

#ifndef _TOOLKIT_HEADERS_TIME_H
#define _TOOLKIT_HEADERS_TIME_H

#include <ABE/basic/Types.h>

__BEGIN_DECLS 

// get system time in usecs since Epoch
// For time measurement and timmer.
__ABE_EXPORT int64_t SystemTimeNs();

#define SystemTime()        SystemTimeNs()
#define SystemTimeUs()      (SystemTimeNs() / 1000)
#define SystemTimeMs()      (SystemTimeNs() / 1000000)

/**
 * suspend thread execution for an interval, @see man(2) nanosleep
 * @return return true on sleep complete, return false if was interrupted by signal
 * @note not all sleep implementation on different os will have guarantee.
 */
__ABE_EXPORT bool SleepNs(int64_t ns);
#define Sleep(ns)           SleepNs(ns)
#define SleepUs(us)         SleepNs(us * 1000LL)
#define SleepMs(ms)         SleepNs(ms * 1000000LL)

/**
 * suspend thread execution for an interval, guarantee time elapsed
 */
__ABE_EXPORT void SleepTimeNs(int64_t ns);
#define SleepTime(ns)       SleepTimeNs(ns)
#define SleepTimeUs(us)     SleepTimeNs(us * 1000LL)
#define SleepTimeMs(ms)     SleepTimeNs(ms * 1000000LL)

__END_DECLS

#endif // _TOOLKIT_HEADERS_TIME_H
