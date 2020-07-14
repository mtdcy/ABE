/******************************************************************************
 * Copyright (c) 2020, Chen Fang <mtdcy.chen@gmail.com>
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


// File:    Mutex.cpp
// Author:  mtdcy.chen
// Changes:
//          1. 20181112     initial version
//

#define LOG_TAG   "Mutex"
#include "ABE/core/Log.h"
#include "ABE/core/Mutex.h"

#include <errno.h>
#include "compat/pthread.h"

__BEGIN_NAMESPACE_ABE

Mutex::Mutex(Bool recursive) {
    pthread_mutexattr_t attr;
    CHECK_EQ(pthread_mutexattr_init(&attr), 0);
    if (recursive)
        CHECK_EQ(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE), 0);
    else
        CHECK_EQ(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK), 0);
    CHECK_EQ(pthread_mutex_init(&mLock, Nil), 0);
    CHECK_EQ(pthread_mutexattr_destroy(&attr), 0);
}

Mutex::~Mutex() {
    CHECK_EQ(pthread_mutex_destroy(&mLock), 0);
}

void Mutex::lock() {
    CHECK_EQ(pthread_mutex_lock(&mLock), 0);
}

void Mutex::unlock() {
    CHECK_EQ(pthread_mutex_unlock(&mLock), 0);
}

Bool Mutex::tryLock() {
    return pthread_mutex_trylock(&mLock) == 0;
}

#if 0
RWLock::RWLock() {
    CHECK_EQ(pthread_rwlock_init(&mLock, Nil), 0);
}

RWLock::~RWLock() {
    CHECK_EQ(pthread_rwlock_destroy(&mLock), 0);
}

void RWLock::lock(Bool write) {
    if (write)
        CHECK_EQ(pthread_rwlock_wrlock(&mLock), 0);
    else
        CHECK_EQ(pthread_rwlock_rdlock(&mLock), 0);
}

Bool RWLock::tryLock(Bool write) {
    if (write)
        return pthread_rwlock_trywrlock(&mLock) == 0;
    else
        return pthread_rwlock_tryrdlock(&mLock) == 0;
}

void RWLock::unlock(Bool write) {
    CHECK_EQ(pthread_rwlock_unlock(&mLock), 0);
}
#endif

///////////////////////////////////////////////////////////////////////////
Condition::Condition() {
    pthread_condattr_t attr;
    CHECK_EQ(pthread_condattr_init(&attr), 0);
#if HAVE_PTHREAD_CONDATTR_SETCLOCK
    CHECK_EQ(pthread_condattr_setclock(&attr, CLOCK_MONOTONIC), 0);
#endif
    CHECK_EQ(pthread_cond_init(&mWait, &attr), 0);
}

Condition::~Condition() {
    CHECK_EQ(pthread_cond_destroy(&mWait), 0);
}

void Condition::wait(Mutex& lock) {
    CHECK_EQ(pthread_cond_wait(&mWait, &(lock.mLock)), 0);
}

Bool Condition::waitRelative(Mutex& lock, Time after) {
    struct timespec ts;
    ts.tv_sec  = after.nseconds() / 1000000000;
    ts.tv_nsec = after.nseconds() % 1000000000;
#if HAVE_PTHREAD_COND_TIMEDWAIT_RELATIVE_NP
    Int rt = pthread_cond_timedwait_relative_np(&mWait, &lock.mLock, &ts);
    if (rt == ETIMEDOUT)    return True;
    else                    return False;
#else
    struct timespec abs;
    clock_gettime(CLOCK_MONOTONIC, &abs);
    abs.tv_nsec += ts.tv_nsec;
    abs.tv_sec  += ts.tv_sec;
    if (abs.tv_nsec >= 1000000000LL) {
        abs.tv_sec  += abs.tv_nsec / 1000000000LL;
        abs.tv_nsec %= 1000000000LL;
    }
    return pthread_cond_timedwait(&mWait, &lock.mLock, &abs);
#endif
}

void Condition::signal() {
    CHECK_EQ(pthread_cond_signal(&mWait), 0);
}

void Condition::broadcast() {
    CHECK_EQ(pthread_cond_broadcast(&mWait), 0);
}

__END_NAMESPACE_ABE
