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


// File:    Mutex.h
// Author:  mtdcy.chen
// Changes: 
//          1. 20160701     initial version
//

#ifndef _TOOLKIT_HEADERS_MUTEX_H
#define _TOOLKIT_HEADERS_MUTEX_H

#include <ABE/basic/Types.h>
#include <pthread.h>

#ifdef __cplusplus
__BEGIN_NAMESPACE_ABE

class Condition;
class __ABE_EXPORT Mutex {
    public:
        Mutex(bool recursive = false);
        ~Mutex();

    public:
        void    lock();
        void    unlock();
        bool    tryLock();      ///< return true on success

    private:
        friend class    Condition;

        pthread_mutex_t mLock;

    private: 
        DISALLOW_EVILS(Mutex);
};

class __ABE_EXPORT AutoLock {
    public:
        __ABE_INLINE AutoLock(Mutex& lock) : mLock(lock)    { mLock.lock(); }
        __ABE_INLINE AutoLock(Mutex* lock) : mLock(*lock)   { mLock.lock(); }
        __ABE_INLINE ~AutoLock()                            { mLock.unlock(); }

    private:
        Mutex&  mLock;

    private:
        DISALLOW_EVILS(AutoLock);
};

class __ABE_EXPORT Condition {
    public:
        Condition();
        ~Condition();
        void    wait(Mutex& lock);
        /**
         * @param nsecs relative time to wait
         * @return return true on timeout, other wise return false
         */
        bool    waitRelative(Mutex& lock, int64_t nsecs);
        void    signal();
        void    broadcast();

    private:
        pthread_cond_t  mWait;

    private:
        DISALLOW_EVILS(Condition);
};

class __ABE_EXPORT RWLock {
    public:
        RWLock();
        ~RWLock();

        void    lock(bool write = false);
        void    unlock(bool write = false);
        bool    tryLock(bool write = false);

    private:
        pthread_rwlock_t    mLock;

    private:
        DISALLOW_EVILS(RWLock);
};

__END_NAMESPACE_ABE
#endif // __cplusplus

#endif // _TOOLKIT_HEADERS_MUTEX_H
