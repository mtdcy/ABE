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


// File:    Thread.cpp
// Author:  mtdcy.chen
// Changes:
//          1. 20160701     initial version
//

#define LOG_TAG   "Thread"
//#define LOG_NDEBUG 0
#include "basic/Log.h"
#include "basic/compat/pthread.h"

#include "stl/List.h"
#include "stl/Vector.h"

#include "basic/Time.h"
#include "tools/Mutex.h"
#include "tools/Thread.h"

#include "basic/private/atomic.h"

// pthread_mutex_t/pthread_cond_t
#include <pthread.h>
// https://stackoverflow.com/questions/24854580/how-to-properly-suspend-threads

#define JOINABLE 1

__BEGIN_NAMESPACE_ABE

enum eThreadIntState {
    kThreadIntNew,
    kThreadIntReady,
    kThreadIntRunning,      // set by client during start
    kThreadIntWaiting,
    kThreadIntTerminating,  // set by client during terminating
    kThreadIntTerminated,
};

struct __ABE_HIDDEN SharedThread : public SharedObject {
    // static context, only writable during kThreadInitial
    eThreadType             mType;
    String                  mName;
    pthread_t               mNativeHandler;
    sp<Runnable>            mRoutine;

    // mutable context, access with lock
    Mutex                   mLock;
    Condition               mWait;
    eThreadIntState         mState;
    bool                    mReadyToRun;
    bool                    mJoinable;
    bool                    mRequestExiting;

    __ABE_INLINE SharedThread(const String& name,
            const eThreadType type,
            const sp<Runnable>& routine) :
        SharedObject(),
        mType(type), mName(name), mNativeHandler(NULL), mRoutine(routine),
        mState(kThreadIntNew), mReadyToRun(false), mJoinable(false),
        mRequestExiting(false)
    {
    }

    __ABE_INLINE virtual ~SharedThread() {
        CHECK_EQ(mState, kThreadIntTerminated);
    }

    __ABE_INLINE bool wouldBlock() {
        return pthread_equal(mNativeHandler, pthread_self());
    }

    static void* ThreadEntry(void * user) {
        SharedThread * thiz = static_cast<SharedThread*>(user);
        // retain object
        thiz->RetainObject();
        // execution
        thiz->execution();
        thiz->ReleaseObject();

        pthread_exit(NULL);
    }

    __ABE_INLINE void setState_l(eThreadIntState state) {
        mState  = state;
        mWait.signal();
    }

    __ABE_INLINE void execution() {
        // local setup
        mLock.lock();
        // simulate pthread_create_suspended_np()
        CHECK_EQ(mState, kThreadIntNew);
        setState_l(kThreadIntReady);
        while (!mReadyToRun && !mRequestExiting) {
            mWait.wait(mLock);          // wait until client is ready
        }

        // set thread properties
        if (!mName.empty()) pthread_setname_mpx(mName.c_str());
        // TODO: interpret thread type to nice & priority

        setState_l(kThreadIntRunning);
        mLock.unlock();

        pthread_yield_mpx();            // give client chance to hold lock

        mLock.lock();
        if (mReadyToRun) {              // in case join() or detach() without run
            mLock.unlock();
            mRoutine->run();
            mLock.lock();
        }

        // local cleanup
        setState_l(kThreadIntTerminated);
        mLock.unlock();
        DEBUG("%s: end...", mName.c_str());
    }

    // client have to retain this object before start()
    __ABE_INLINE void start() {
        CHECK_FALSE(wouldBlock());

        AutoLock _l(mLock);
        CHECK_EQ(mState, kThreadIntNew);

        // create new thread
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        // use joinable thread.
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

        switch (pthread_create(&mNativeHandler, &attr, ThreadEntry, this)) {
            case EAGAIN:
                FATAL("%s: Insufficient resources to create another thread.", mName.c_str());
                break;
            case EINVAL:
                FATAL("%s: Invalid settings in attr.", mName.c_str());
                break;
            default:
                FATAL("%s: pthread_create failed", mName.c_str());
                break;
            case 0:
                break;
        }

        mJoinable = true;
        // wait until thread is ready
        while (mState < kThreadIntReady) {
            mWait.wait(mLock);
        }
        pthread_attr_destroy(&attr);
    }

    __ABE_INLINE void run() {
        AutoLock _l(mLock);
        CHECK_LE(mState, kThreadIntReady);
        CHECK_FALSE(mReadyToRun);
        mReadyToRun     = true;
        mWait.signal();

        // wait until thread leaving ready state
        while (mState <= kThreadIntReady) {
            mWait.wait(mLock);
        }
    }

    // wait this thread's job done and join it
    __ABE_INLINE void join() {
        mLock.lock();
        CHECK_TRUE(mJoinable);
        mRequestExiting         = true;
        mJoinable               = false;
        mWait.signal();
        mLock.unlock();

        // pthread_join without lock
        if (wouldBlock()) {
            INFO("%s: join inside thread", mName.c_str());
            pthread_detach(mNativeHandler);
        } else {
            switch (pthread_join(mNativeHandler, NULL)) {
                case EDEADLK:
                    FATAL("%s: A deadlock was detected", mName.c_str());
                    break;
                case EINVAL:
                    ERROR("%s: not a joinable thread.", mName.c_str());
                    break;
                case ESRCH:
                    ERROR("%s: thread could not be found.", mName.c_str());
                    break;
                default:
                    ERROR("%s: pthread_detach failed.", mName.c_str());
                    break;
                case 0:
                    break;
            }
        }
    }

    __ABE_INLINE void detach(bool wait) {
        mLock.lock();
        mRequestExiting         = true;
        mJoinable               = false;
        mWait.signal();
        mLock.unlock();

        // detach without lock
        switch (pthread_detach(mNativeHandler)) {
            case EINVAL:
                ERROR("%s: not a joinable thread.", mName.c_str());
                break;
            case ESRCH:
                ERROR("%s: thread could not be found.", mName.c_str());
                break;
            default:
                ERROR("%s: pthread_detach failed.", mName.c_str());
                break;
            case 0:
                break;
        }
    }
};

//////////////////////////////////////////////////////////////////////////////////
static sp<Runnable> once_runnable;
static void once_routine() {
    once_runnable->run();
}
void Thread::Once(const sp<Runnable>& runnable) {
    once_runnable = runnable;
    pthread_once_t once = PTHREAD_ONCE_INIT;
    // FIXME: put runnable in global variable is not right
    CHECK_EQ(pthread_once(&once, once_routine), 0);
    // on return from pthread_once, once_routine has completed
    once_runnable.clear();
}

static volatile int g_thread_id = 0;
static String MakeThreadName() {
    return String::format("thread%zu", (size_t)atomic_add(&g_thread_id, 1));
}

Thread Thread::Null = Thread();

Thread::Thread(const sp<Runnable>& runnable, const eThreadType type) : mShared(NULL)
{
    SharedThread *shared = new SharedThread(MakeThreadName(), type, runnable);
    mShared = shared;   // must retain object before start

    shared->start();
    // execute when run()
}

Thread::~Thread() {
}

Thread& Thread::setName(const String& name) {
    sp<SharedThread> shared = mShared;
    AutoLock _l(shared->mLock);
    CHECK_TRUE(shared->mState <= kThreadIntReady);
    shared->mName = name;
    return *this;
}

String Thread::name() const {
    sp<SharedThread> shared = mShared;
    AutoLock _l(shared->mLock);
    return shared->mName;
}

Thread& Thread::setType(const eThreadType type) {
    sp<SharedThread> shared = mShared;
    AutoLock _l(shared->mLock);
    CHECK_TRUE(shared->mState <= kThreadIntReady);
    shared->mType = type;
    return *this;
}

eThreadType Thread::type() const {
    sp<SharedThread> shared = mShared;
    AutoLock _l(shared->mLock);
    return shared->mType;
}

Thread& Thread::run() {
    sp<SharedThread> shared = mShared;
    shared->run();
    return *this;
}

bool Thread::joinable() const {
    sp<SharedThread> shared = mShared;
    AutoLock _l(shared->mLock);
    return shared->mJoinable;
}

void Thread::join() {
    sp<SharedThread> shared = mShared;
    shared->join();
}

void Thread::detach() {
    sp<SharedThread> shared = mShared;
    shared->detach(false);
}

pthread_t Thread::native_thread_handle() const {
    sp<SharedThread> shared = mShared;
    AutoLock _l(shared->mLock);
    return shared->mNativeHandler;
}

Thread::eThreadState Thread::state() const {
    sp<SharedThread> shared = mShared;
    AutoLock _l(shared->mLock);
    switch (shared->mState) {
        case kThreadIntNew:
        case kThreadIntReady:
            return kThreadInitializing;
        case kThreadIntRunning:
        case kThreadIntWaiting:
            return kThreadRunning;
        case kThreadIntTerminating:
        case kThreadIntTerminated:
            return kThreadTerminated;
        default:
            FATAL("FIXME");
    }
}

void Thread::Yield() {
    pthread_yield_mpx();
}

__END_NAMESPACE_ABE
