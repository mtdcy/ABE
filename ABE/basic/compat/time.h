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


// File:    time.h
// Author:  mtdcy.chen
// Changes: 
//          1. 20161012     initial version
//


#ifndef __toolkit_compat_time_h
#define __toolkit_compat_time_h 

#include <ABE/basic/Types.h> 

#ifdef __linux__
#define __need_timespec 
#endif
#include <time.h>
#include <sys/time.h>

__BEGIN_DECLS

// NOTE: 
// not all sleep implementation on different os will have guarantee.
// So, we should avoid to use sleep.
//
// sleep 
// usleep
// nanosleep

// for interval measurements
// get current time since some arbitrary
// using monotonic clock if supported.
void            relative_time(struct timespec *ts);

// get current time since Epoch
// affected by system time-of-day clock changing, including NTP
// using CLOCK_REALTIME if supported
void            absolute_time(struct timespec *ts);

// for function like pthread_cond_timedwait
void            absolute_time_later(struct timespec *ts, uint64_t nsecs);

#define         nseconds(ts) ((ts).tv_sec * 1000000000LL + (ts).tv_nsec)
#define         useconds(ts) ((ts).tv_sec * 1000000LL + (ts).tv_nsec / 1000)
#define         mseconds(ts) ((ts).tv_sec * 1000LL + (ts).tv_nsec / 1000000LL)
#define         seconds(ts)  ((ts).tv_sec)

__END_DECLS 

#endif // __toolkit_compat_time_h 
