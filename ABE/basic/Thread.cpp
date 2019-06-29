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

#include "stl/List.h"
#include "stl/Vector.h"

#include "basic/Time.h"
#include "basic/Mutex.h"
#include "basic/Thread.h"

#include "basic/Atomic.h"
#include <sched.h>

#include "compat/pthread.h"

#define JOINABLE 1

__BEGIN_NAMESPACE_ABE

enum eThreadIntState {
    kThreadIntNew,
    kThreadIntReady,
    kThreadIntReadyToRun,   // set by client when ready to run
    kThreadIntRunning,
    kThreadIntTerminating,  // set by client during terminating
    kThreadIntTerminated,
};

static const char * NAMES[] = {
    "Lowest",
    "Backgroud",
    "Normal",
    "Foreground",
    "System",
    "Kernel",
    "Realtime",
    "Realtime",
    "Highest"
};

// interpret thread type to nice & priority
// kThreadLowest - kThreadForegroud :   SCHED_OTHER
// kThreadSystem - kThreadKernel:       SCHED_FIFO
// kThreadRealtime - kThreadHighest:    SCHED_RR
static ABE_INLINE void SetThreadType(const String& name, eThreadType type) {
    sched_param old;
    sched_param par;
    int policy;
    pthread_getschedparam(pthread_self(), &policy, &old);
    DEBUG("%s: policy %d, sched_priority %d", name.c_str(), policy, old.sched_priority);
    if (type < kThreadSystem) {
        policy = SCHED_OTHER;
        int min = sched_get_priority_min(SCHED_OTHER);
        int max = sched_get_priority_max(SCHED_OTHER);
        // make sure kThreadDefault map to default priority
        par.sched_priority = old.sched_priority;
        if (type >= kThreadDefault)
            par.sched_priority += ((max - old.sched_priority) * (type - kThreadDefault)) / (kThreadSystem - kThreadDefault);
        else
            par.sched_priority += ((min - old.sched_priority) * (type - kThreadDefault)) / (kThreadLowest - kThreadDefault);
    } else if (type < kThreadRealtime) {
        policy = SCHED_FIFO;
        int min = sched_get_priority_min(SCHED_FIFO);
        int max = sched_get_priority_max(SCHED_FIFO);
        par.sched_priority = min + ((max - min) * (type - kThreadSystem)) / (kThreadRealtime - kThreadSystem);
    } else {
        policy = SCHED_RR;
        int min = sched_get_priority_min(SCHED_RR);
        int max = sched_get_priority_max(SCHED_RR);
        par.sched_priority = min + ((max - min) * (type - kThreadRealtime)) / (kThreadHighest - kThreadRealtime);
    }
    int err = pthread_setschedparam(pthread_self(), policy, &par);
    switch (err) {
        case 0:
            DEBUG("%s: %s => policy %d, sched_priority %d",
                    name.c_str(), NAMES[type/16], policy, par.sched_priority);
            break;
        case EINVAL:
            ERROR("%s: %s => %d %d failed, Invalid value for policy.",
                    name.c_str(), NAMES[type/16], policy, par.sched_priority);
            break;
        case ENOTSUP:
            ERROR("%s: %s => %d %d failed, Invalid value for scheduling parameters.",
                    name.c_str(), NAMES[type/16], policy, par.sched_priority);
            break;
        case ESRCH:
            ERROR("%s: %s => %d %d failed, Non-existent thread thread.",
                    name.c_str(), NAMES[type/16], policy, par.sched_priority);
            break;
        default:
            ERROR("%s: %s => %d %d failed, pthread_setschedparam failed, err %d|%s",
                    name.c_str(), NAMES[type/16], policy, par.sched_priority, err, strerror(err));
            break;
    }
}

struct SharedThread : public SharedObject {
    // static context, only writable during kThreadInitial
    eThreadType             mType;
    String                  mName;
    pthread_t               mNativeHandler;     // no initial value to pthread_t
    Object<Runnable>        mRoutine;
    
    // mutable context, access with lock
    mutable Mutex           mLock;
    Condition               mWait;
    eThreadIntState         mState;
    
    SharedThread(const String& name,
                 const eThreadType type,
                 const Object<Runnable>& routine) : SharedObject(),
    mType(type), mName(name), mRoutine(routine), mState(kThreadIntNew) { }
    
    virtual ~SharedThread() {
        CHECK_EQ(mState, kThreadIntTerminated);
    }
    
    ABE_INLINE bool wouldBlock() {
        return pthread_equal(mNativeHandler, pthread_self());
    }
    
    ABE_INLINE bool joinable() const {
        AutoLock _l(mLock);
        return mState >= kThreadIntReady && mState < kThreadIntTerminating;
    }
    
    virtual void start() = 0;
    virtual void run() = 0;
    virtual void join() = 0;
    virtual void detach() = 0;
};

struct NormalThread : public SharedThread {
    ABE_INLINE NormalThread(const String& name,
            const eThreadType type,
            const Object<Runnable>& routine) :
        SharedThread(name, type, routine) { }

    static void* ThreadEntry(void * user) {
        NormalThread * thiz = static_cast<NormalThread*>(user);
        // retain object
        thiz->RetainObject();
        // execution
        thiz->execution();
        thiz->ReleaseObject();

        pthread_exit(NULL);
        return NULL;    // just fix build warnings
    }

    ABE_INLINE void setState_l(eThreadIntState state) {
        mState  = state;
        mWait.signal();
    }

    ABE_INLINE void execution() {
        // local setup
        AutoLock _l(mLock);
        
        // simulate pthread_create_suspended_np()
        CHECK_EQ(mState, kThreadIntNew);
        setState_l(kThreadIntReady);
        while (mState == kThreadIntReady) mWait.wait(mLock);    // wait until client is ready

        if (mState == kThreadIntReadyToRun) {                   // in case join() or detach() without run
            // set thread properties
            pthread_setname(mName.c_str());
            SetThreadType(mName, mType);

            setState_l(kThreadIntRunning);
            mLock.unlock();

            pthread_yield();            // give client chance to hold lock

            mLock.lock();
            mLock.unlock();
            mRoutine->run();
            mLock.lock();
        }

        // local cleanup
        while (mState != kThreadIntTerminating) {
            mWait.wait(mLock);
        }
        setState_l(kThreadIntTerminated);
        DEBUG("%s: end...", mName.c_str());
    }

    // client have to retain this object before start()
    virtual void start() {
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

        // wait until thread is ready
        while (mState < kThreadIntReady) {
            mWait.wait(mLock);
        }
        pthread_attr_destroy(&attr);
    }

    virtual void run() {
        AutoLock _l(mLock);
        CHECK_LE(mState, kThreadIntReady);
        setState_l(kThreadIntReadyToRun);

        // wait until thread leaving ready state
        while (mState < kThreadIntRunning) {
            mWait.wait(mLock);
        }
    }

    // wait this thread's job done and join it
    virtual void join() {
        AutoLock _l(mLock);
        CHECK_TRUE(mState >= kThreadIntReady && mState < kThreadIntTerminating);
        setState_l(kThreadIntTerminating);
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
        
        mLock.lock();
        while (mState != kThreadIntTerminated) mWait.wait(mLock);
    }

    virtual void detach() {
        mLock.lock();
        setState_l(kThreadIntTerminating);
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

struct MainThread : public SharedThread {
    MainThread() : SharedThread("main", kThreadNormal, NULL) {
        mNativeHandler = pthread_self();
        pthread_setname("main");
    }
    
    virtual ~MainThread() {
        mState = kThreadIntTerminated;
    }
    
    virtual void start() {
        mState = kThreadIntReady;
    }
    
    virtual void run() {
        CHECK_TRUE(mState == kThreadIntReady);
        mState = kThreadIntRunning;
    }
    
    virtual void join() {
        CHECK_TRUE(mState >= kThreadIntReady && mState < kThreadIntTerminating);
        mState = kThreadIntTerminated;
    }
    
    virtual void detach() {
        CHECK_TRUE(mState == kThreadIntRunning);
        mState = kThreadIntTerminated;
    }
};

//////////////////////////////////////////////////////////////////////////////////
static Atomic<int> g_thread_id;
static String MakeThreadName() {
    return String::format("thread%zu", ++g_thread_id);
}

Thread& Thread::Main() {
    static Thread _main = Thread();
    if (_main.mShared == NULL) {
        CHECK_TRUE(pthread_main(), "thread is NOT initialized in main()");
        Object<SharedThread> shared = new MainThread;
        _main.mShared = shared;
        
        shared->start();
    }
    return _main;
}

// static
Thread Thread::Null = Thread();

Thread::Thread(const Object<Runnable>& runnable, const eThreadType type) : mShared(NULL) {
    Object<SharedThread> shared = new NormalThread(MakeThreadName(), type, runnable);
    shared->start();
    
    mShared = shared;
    // execute when run()
}

Thread::Thread(const Thread& rhs) : mShared(NULL) {
    if (rhs.mShared.isNIL()) return;
    
    Object<SharedThread> from = rhs.mShared;
    Object<SharedThread> shared = new NormalThread(MakeThreadName(), from->mType, from->mRoutine);
    shared->start();
    
    mShared = shared;
}

Thread& Thread::setName(const String& name) {
    Object<SharedThread> shared = mShared;
    AutoLock _l(shared->mLock);
    CHECK_TRUE(shared->mState <= kThreadIntReady);
    shared->mName = name;
    return *this;
}

String& Thread::name() const {
    Object<SharedThread> shared = mShared;
    AutoLock _l(shared->mLock);
    return shared->mName;
}

Thread& Thread::setType(const eThreadType type) {
    Object<SharedThread> shared = mShared;
    AutoLock _l(shared->mLock);
    CHECK_TRUE(shared->mState <= kThreadIntReady);
    shared->mType = type;
    return *this;
}

eThreadType Thread::type() const {
    Object<SharedThread> shared = mShared;
    AutoLock _l(shared->mLock);
    return shared->mType;
}

Thread& Thread::run() {
    Object<SharedThread> shared = mShared;
    shared->run();
    return *this;
}

bool Thread::joinable() const {
    Object<SharedThread> shared = mShared;
    return shared->joinable();
}

void Thread::join() {
    Object<SharedThread> shared = mShared;
    shared->join();
}

void Thread::detach() {
    Object<SharedThread> shared = mShared;
    shared->detach();
}

pthread_t Thread::native_thread_handle() const {
    Object<SharedThread> shared = mShared;
    AutoLock _l(shared->mLock);
    return shared->mNativeHandler;
}

Thread::eThreadState Thread::state() const {
    Object<SharedThread> shared = mShared;
    AutoLock _l(shared->mLock);
    switch (shared->mState) {
        case kThreadIntNew:
        case kThreadIntReady:
            return kThreadInitializing;
        case kThreadIntReadyToRun:
        case kThreadIntRunning:
            return kThreadRunning;
        case kThreadIntTerminating:
        case kThreadIntTerminated:
            return kThreadTerminated;
        default:
            FATAL("FIXME");
    }
    return kThreadTerminated; // just fix build warnings
}

void Thread::Yield() {
    pthread_yield();
}

String Thread::Name() {
    char name[16];
    if (pthread_getname_np(pthread_self(), name, 16) < 0 ||  name[0] == '\0') {
        pid_t id = pthread_gettid();
        return String(id);
    }
    return String(name);
}

__END_NAMESPACE_ABE
