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


// File:    pthread.h
// Author:  mtdcy.chen
// Changes: 
//          1. 20161012     initial version
//

#ifndef __ABE_basic_pthread_compat_h
#define __ABE_basic_pthread_compat_h

#define _DARWIN_C_SOURCE
#include <pthread.h>
#include "Config.h"
#include <sys/types.h>  // pid_t gettid
#include <unistd.h>     // getpid

#ifndef HAVE_GETTID
#include <sys/syscall.h>
#endif

__BEGIN_DECLS

// for portable reason, we are not suppose to using the _np part
// bellow is something we want to make portable.
// by removing _np suffix make it won't have name conflict or confusion

// the name is restricted to 16 characters, including the terminating null byte
static inline int pthread_setname(const char * name) {
    return pthread_setname_np(pthread_self(), name);
}

static inline int pthread_getname(pthread_t thread, char * name, size_t len) {
    return pthread_getname_np(thread, name, len);
}

// no return value
//#define pthread_yield()                     pthread_yield_np()

// get self tid
#define pthread_gettid()                      gettid()
static inline pid_t pthread_gettid() {
#ifdef HAVE_GETTID
    return gettid();
#else
    return syscall(SYS_gettid);
#endif
}

// return 1 if current thread is main thread
static inline int pthread_main() {
    return getpid() == pthread_gettid();
}

#define PTHREAD_COND_CLOCK_ID   CLOCK_MONOTONIC
static inline int pthread_cond_timedwait_relative(pthread_cond_t * cond, pthread_mutex_t * mutex, 
                                                    const struct timespec * ts) {
#ifdef HAVE_PTHREAD_COND_TIMEDWAIT_RELATIVE 
    return pthread_cond_timedwait_relative_np(cond, mutex, ts);
#else
    struct timespec abs;
    clock_gettime(PTHREAD_COND_CLOCK_ID, &abs);
    abs.tv_nsec += ts->tv_nsec;
    abs.tv_sec  += ts->tv_sec;
    if (abs.tv_nsec >= 1000000000LL) {
        abs.tv_sec  += abs.tv_nsec / 1000000000LL;
        abs.tv_nsec %= 1000000000LL;
    }
    return pthread_cond_timedwait(cond, mutex, &abs);
#endif
}

__END_DECLS 

#endif // __ABE_basic_pthread_compat_h

