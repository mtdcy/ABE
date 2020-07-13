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


#ifndef ABE_compat_pthread_h
#define ABE_compat_pthread_h 

#include "core/Types.h"
#include "Config.h"

#if defined(_WIN32)
#include <Windows.h>
/* use light weight mutex/condition variable API for Windows Vista and later */
typedef SRWLOCK                 pthread_mutex_t;
typedef CONDITION_VARIABLE      pthread_cond_t;

#define PTHREAD_MUTEX_INITIALIZER       SRWLOCK_INIT
#define PTHREAD_COND_INITIALIZER        CONDITION_VARIABLE_INIT

#define InitializeCriticalSection(x)    InitializeCriticalSectionEx(x, 0, 0)
#define WaitForSingleObject(a, b)       WaitForSingleObjectEx(a, b, FALSE)

static inline Int pthread_mutex_init(pthread_mutex_t *m, void *attr) {
    InitializeSRWLock(m);
    return 0;
}
static inline Int pthread_mutex_destroy(pthread_mutex_t *m) {
    /* Unlocked SWR locks use no resources */
    return 0;
}
static inline Int pthread_mutex_lock(pthread_mutex_t *m) {
    AcquireSRWLockExclusive(m);
    return 0;
}
static inline Int pthread_mutex_unlock(pthread_mutex_t *m) {
    ReleaseSRWLockExclusive(m);
    return 0;
}

static inline Int pthread_cond_init(pthread_cond_t *cond,
                                    const void *unused_attr) {
    InitializeConditionVariable(cond);
    return 0;
}

/* native condition variables do not destroy */
static inline Int pthread_cond_destroy(pthread_cond_t *cond) { return 0; }

static inline Int pthread_cond_broadcast(pthread_cond_t *cond) {
    WakeAllConditionVariable(cond);
    return 0;
}

static inline Int pthread_cond_wait(pthread_cond_t *cond,
                                    pthread_mutex_t *mutex) {
    SleepConditionVariableSRW(cond, mutex, INFINITE, 0);
    return 0;
}

static inline Int pthread_cond_timedwait_relative(pthread_cond_t* cond,
                                                  pthread_mutex_t* mutex,
                                                  struct timespec ts) {
    DWORD dwMilliseconds = ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
    SleepConditionVariableSRW(cond, mutex, dwMilliseconds, 0);
    return 0;
}

static inline Int pthread_cond_signal(pthread_cond_t *cond) {
    WakeConditionVariable(cond);
    return 0;
}

#else

#include <pthread.h>
#include <sys/types.h>

#if defined(__linux__)
#include <unistd.h>
#include <sys/syscall.h>
#endif

static ABE_INLINE void pthread_setname(const Char * name) {
#if HAVE_PTHREAD_SETNAME_NP
#if defined(__APPLE__)
    pthread_setname_np(name);
#else
    pthread_setname_np(pthread_self(), name);
#endif
#else
#error "missing pthread_setname_np"
#endif
}

/**
 * on Linux, the name len >= 16, or it will fail
 */
static ABE_INLINE void pthread_getname(Char * name, UInt32 len) {
#if HAVE_PTHREAD_GETNAME_NP
    pthread_getname_np(pthread_self(), name, len);
#else
#error "missing pthread_getname_np"
#endif
}

static ABE_INLINE pid_t pthread_gettid() {
#if HAVE_GETTID
    return gettid();
#elif defined(__APPLE__)
    UInt64 id;
    pthread_threadid_np(pthread_self(), &id);
    return id;
#else
    return syscall(SYS_gettid);
#endif
}

static ABE_INLINE Bool pthread_main() {
#if HAVE_PTHREAD_MAIN_NP
    return pthread_main_np() != 0;
#else
    return getpid() == pthread_gettid();
#endif
}

#if defined(__APPLE__)
#define pthread_yield       pthread_yield_np
#endif

#endif

#endif // ABE_compat_pthread_h 
