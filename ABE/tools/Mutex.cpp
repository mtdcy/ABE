/******************************************************************************
  Copyright (c) 2015, mtdcy
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *****************************************************************************/

/******************************************************************************
 * Note: 
 *  This is ...
 *
 * Author:
 *  mtdcy <mtdcy.chen@gmail.com>
 *
 * Changes:
 *
 *  20151205    mtdcy           initial version
 *****************************************************************************/

#if defined(__APPLE__)
#include "basic/compat/pthread_macos.h"
#include "basic/compat/time_macos.h"
#else
#endif

#define LOG_TAG   "Mutex"
#include "ABE/basic/Log.h"
#include "ABE/tools/Mutex.h"

__BEGIN_NAMESPACE_ABE

Mutex::Mutex(bool recursive) {
    pthread_mutexattr_t attr;
    CHECK_EQ(pthread_mutexattr_init(&attr), 0);
    if (recursive)
        CHECK_EQ(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE), 0);
    else
        CHECK_EQ(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK), 0);
    CHECK_EQ(pthread_mutex_init(&mLock, NULL), 0);
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

bool Mutex::tryLock() {
    return pthread_mutex_trylock(&mLock) == 0;
}

RWLock::RWLock() {
    CHECK_EQ(pthread_rwlock_init(&mLock, NULL), 0);
}

RWLock::~RWLock() {
    CHECK_EQ(pthread_rwlock_destroy(&mLock), 0);
}

void RWLock::lock(bool write) {
    if (write)
        CHECK_EQ(pthread_rwlock_wrlock(&mLock), 0);
    else
        CHECK_EQ(pthread_rwlock_rdlock(&mLock), 0);
}

bool RWLock::tryLock(bool write) {
    if (write)
        return pthread_rwlock_trywrlock(&mLock) == 0;
    else
        return pthread_rwlock_tryrdlock(&mLock) == 0;
}

void RWLock::unlock(bool write) {
    CHECK_EQ(pthread_rwlock_unlock(&mLock), 0);
}

///////////////////////////////////////////////////////////////////////////
Condition::Condition() {
    CHECK_EQ(pthread_cond_init(&mWait, NULL), 0);
}

Condition::~Condition() {
    CHECK_EQ(pthread_cond_destroy(&mWait), 0);
}

void Condition::wait(Mutex& lock) {
    CHECK_EQ(pthread_cond_wait(&mWait, &(lock.mLock)), 0);
}

bool Condition::waitRelative(Mutex& lock, int64_t reltime /* ns */) {
    struct timespec ts;
    ts.tv_sec  = reltime / 1000000000;
    ts.tv_nsec = reltime % 1000000000;
    int rt = pthread_timed_wait_relative(&mWait, &lock.mLock, &ts);
    if (rt == ETIMEDOUT)    return true;
    else                    return false;
}

void Condition::signal() {
    CHECK_EQ(pthread_cond_signal(&mWait), 0);
}

void Condition::broadcast() {
    CHECK_EQ(pthread_cond_broadcast(&mWait), 0);
}

__END_NAMESPACE_ABE
