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


// File:    Looper.cpp
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
#include "Looper.h"

// https://stackoverflow.com/questions/24854580/how-to-properly-suspend-threads

__BEGIN_NAMESPACE_ABE

struct __ABE_HIDDEN Job {
    sp<Runnable>    mRoutine;
    int64_t         mTime;

    Job(const sp<Runnable>& routine, int64_t delay) : mRoutine(routine),
    mTime(SystemTimeUs() + (delay < 0 ? 0 : delay)) { }
};

struct __ABE_HIDDEN JobDispatcher : public Runnable {
    // internal context
    String                  mName;

    // mutable context, access with lock
    mutable Mutex           mLock;
    Condition               mWait;
    List<Job>               mJobs;

    bool                    mLooping;
    bool                    mWaitForJobDidFinished;
    Vector<void *>          mUserContext;

    JobDispatcher(const String& name) : Runnable(), mName(name),
    mLooping(true), mWaitForJobDidFinished(false) { }

    void post(const sp<Runnable>& routine, int64_t delay = 0) {
        AutoLock _l(mLock);

        Job job(routine, delay);

        // go through list should be better than sort()
        List<Job>::iterator it = mJobs.begin();
        while (it != mJobs.end() && (*it).mTime < job.mTime) {
            ++it;
        }

        if (it == mJobs.begin()) {
            // have signal before insert(), as insert() will change begin()
            mWait.signal();
        }
        mJobs.insert(it, job);
    }

    void remove(const sp<Runnable>& routine) {
        AutoLock _l(mLock);

        List<Job>::iterator it = mJobs.begin();
        while (it != mJobs.end()) {
            if ((*it).mRoutine == routine) {
                it = mJobs.erase(it);
            } else {
                ++it;
            }
        }
        mWait.signal();
    }

    bool exists(const sp<Runnable>& routine) const {
        AutoLock _l(mLock);

        List<Job>::const_iterator it = mJobs.cbegin();
        for (; it != mJobs.cend(); ++it) {
            if ((*it).mRoutine == routine) {
                return true;
            }
        }
        return false;
    }

    void clear() {
        AutoLock _l(mLock);
        mJobs.clear();
    }

    void terminate(bool wait) {
        AutoLock _l(mLock);
        mLooping                = false;
        mWaitForJobDidFinished  = wait;
        mWait.signal();
    }
};

struct __ABE_HIDDEN NormalJobDispatcher : public JobDispatcher {
    NormalJobDispatcher(const String& name) : JobDispatcher(name) { }

    virtual void run() {
        mLock.lock();

        for (;;) {
            if (mJobs.empty()) {
                if (!mLooping) break;

                mWait.wait(mLock);
                continue;
            }

            Job& head = mJobs.front();
            const int64_t now = SystemTimeUs();
            if (head.mTime > now) {
                bool timeout = mWait.waitRelative(mLock, (head.mTime - now) * 1000);
                if (timeout) {
                    int64_t now = SystemTimeUs();
                    if (now < head.mTime) {
                        ERROR("%s: waitRelative is not working properly", mName.c_str());
                    }
                } else {
                    continue;
                }
            } else {
                DEBUG("%s: message exec delayed %.2f msecs", mName.c_str(), (now - head.time) / 1E3);
            }

            Job cur = head;
            mJobs.pop();

            // execution without lock
            mLock.unlock();
            cur.mRoutine->run();
            mLock.lock();
            // end of execution

            // exit with jobs
            if (!mLooping && !mWaitForJobDidFinished) {
                if (mJobs.size()) {
                    INFO("%s: leave with %zu job behind", mName.c_str(), mJobs.size());
                }
                break;
            }
        }

        mWait.signal();
        mLock.unlock();
    }
};

struct __ABE_HIDDEN MainJobDispatcher : public JobDispatcher {
    MainJobDispatcher() : JobDispatcher("main") { }

    virtual void run() {
        mLock.lock();

        for (;;) {
            if (mJobs.empty()) {
                // XXX: wait for signal instead of exit
                break;
            }

            Job& head = mJobs.front();
            const int64_t now = SystemTimeUs();
            if (head.mTime > now) {
                bool timeout = mWait.waitRelative(mLock, (head.mTime - now) * 1000);
                if (timeout) {
                    int64_t now = SystemTimeUs();
                    if (now < head.mTime) {
                        ERROR("%s: waitRelative is not working properly", mName.c_str());
                    }
                } else {
                    continue;
                }
            } else {
                DEBUG("%s: message exec delayed %.2f msecs", mName.c_str(), (now - head.time) / 1E3);
            }

            Job cur = head;
            mJobs.pop();

            // execution without lock
            mLock.unlock();
            cur.mRoutine->run();
            mLock.lock();
            // end of execution

            // exit with jobs
            if (!mLooping && !mWaitForJobDidFinished) {
                if (mJobs.size()) {
                    INFO("%s: leave with %zu job behind", mName.c_str(), mJobs.size());
                }
                break;
            } else if (IsObjectNotShared()) {
                // object is only retained by tls
                INFO("%s: clients gone", mName.c_str());
                break;
            }
        }

        mWait.signal();
        mLock.unlock();
    }
};

struct __ABE_HIDDEN SharedLooper : public SharedObject {
    sp<JobDispatcher>   mDispatcher;
    Thread *            mThread;

    // for main looper
    SharedLooper() : SharedObject(), mDispatcher(new MainJobDispatcher), mThread(NULL) {
    }

    // for normal looper
    SharedLooper(const String& name, eThreadType type) :
        SharedObject(), mDispatcher(new NormalJobDispatcher(name)), mThread(new Thread(mDispatcher)) {
            mThread->setName(name).setType(type);
        }

    virtual ~SharedLooper() { if (mThread) delete mThread; }
};

static __thread Looper * local_looper = NULL;
static Looper * main_looper = NULL;
struct __ABE_HIDDEN FirstRoutine : public Runnable {
    sp<Looper> self;
    FirstRoutine(const sp<Looper>& _self) : Runnable(), self(_self) { }

    virtual void run() {
        // set tls value
        local_looper = self.get();
    }
};

//////////////////////////////////////////////////////////////////////////////////
sp<Looper> Looper::Current() {
    return local_looper;
}

// main looper without backend thread
sp<Looper> Looper::Main() {
    if (main_looper == NULL) {
        main_looper = new Looper;
        sp<SharedLooper> looper = new SharedLooper;
        main_looper->mShared = looper;
    }
    return main_looper;
}

Looper::Looper(const String& name, const eThreadType& type) : SharedObject(),
    mShared(new SharedLooper(name, type))
{
}

Looper::~Looper() {
    if (this == main_looper)    main_looper = NULL;
}

String& Looper::name() const {
    sp<SharedLooper> looper = mShared;
    return looper->mDispatcher->mName;
}

Thread& Looper::thread() const {
    sp<SharedLooper> looper = mShared;
    CHECK_NULL(looper->mThread, "no backend thread for main looper");
    return *looper->mThread;
}

void Looper::loop() {
    sp<SharedLooper> looper = mShared;
    if (looper->mThread) {
        looper->mDispatcher->post(new FirstRoutine(this));
        looper->mThread->run();
    } else {
        looper->mDispatcher->run();
    }
}

void Looper::terminate(bool wait) {
    sp<SharedLooper> looper = mShared;
    looper->mDispatcher->terminate(wait);
    if (looper->mThread) {
        if (wait)   looper->mThread->join();
        else        looper->mThread->detach();
    }
}

void Looper::post(const sp<Runnable>& routine, int64_t delayUs) {
    sp<SharedLooper> looper = mShared;
    looper->mDispatcher->post(routine, delayUs);
}

void Looper::remove(const sp<Runnable>& routine) {
    sp<SharedLooper> looper = mShared;
    looper->mDispatcher->remove(routine);
}

bool Looper::exists(const sp<Runnable>& routine) const {
    sp<SharedLooper> looper = mShared;
    return looper->mDispatcher->exists(routine);
}

void Looper::flush() {
    sp<SharedLooper> looper = mShared;
    looper->mDispatcher->clear();
}

String Looper::string() const {
    sp<SharedLooper> looper = mShared;

    String info = String::format("looper %s: jobs %zu",
            looper->mDispatcher->mName.c_str(),
            looper->mDispatcher->mJobs.size());
#if 0
    size_t index = 0;
    List<Job>::const_iterator it = shared->mJobs.cbegin();
    for (; it != shared->mJobs.cend(); ++it) {
        const Job& job = *it;
        info.append("\n");
        info.append(String::format("Job [%zu]: ***", index++));
    }
#endif
    return info;
}

size_t Looper::bind(void * user) {
    sp<SharedLooper> looper = mShared;

    AutoLock _l(looper->mDispatcher->mLock);
    looper->mDispatcher->mUserContext.push(user);
    return looper->mDispatcher->mUserContext.size() - 1;
}

void Looper::bind(size_t id, void *user) {
    sp<SharedLooper> looper = mShared;

    AutoLock _l(looper->mDispatcher->mLock);
    CHECK_LE(id, looper->mDispatcher->mUserContext.size());
    if (id == looper->mDispatcher->mUserContext.size()) {
        looper->mDispatcher->mUserContext.push(user);
    } else {
        looper->mDispatcher->mUserContext[id] = user;
    }
}

void * Looper::user(size_t id) const {
    sp<SharedLooper> looper = mShared;

    AutoLock _l(looper->mDispatcher->mLock);
    if (id >= looper->mDispatcher->mUserContext.size())
        return NULL;
    return looper->mDispatcher->mUserContext[id];
}

__END_NAMESPACE_ABE

//////////////////////////////////////////////////////////////////////////////////
USING_NAMESPACE_ABE

extern "C" {

    struct __ABE_HIDDEN UserRunnable : public Runnable {
        void (*callback)(void *);
        void * user;
        UserRunnable(void (*Callback)(void *), void * User) : Runnable(),
        callback(Callback), user(User) { }
        virtual void run() { callback(user); }
    };

    Runnable *  SharedRunnableCreate(void (*Callback)(void *), void * User) {
        Runnable * shared = new UserRunnable(Callback, User);
        return (Runnable *)shared->RetainObject();
    }

    Looper * SharedLooperCreate(const char * name) {
        sp<Looper> shared = Looper::Create(name);
        return (Looper *)shared->RetainObject();
    }

    void SharedLooperLoop(Looper * shared) {
        shared->loop();
    }

    void SharedLooperTerminate(Looper * shared) {
        shared->terminate();
    }

    void SharedLooperTerminateAndWait(Looper * shared) {
        shared->terminate(true);
    }

    void SharedLooperPostRunnable(Looper * shared, Runnable * run) {
        shared->post(run);
    }

    void SharedLooperPostRunnableWithDelay(Looper * shared, Runnable * run, int64_t delay) {
        shared->post(run, delay);
    }

    void SharedLooperRemoveRunnable(Looper * shared, Runnable * run) {
        shared->remove(run);
    }

    bool SharedLooperFindRunnable(Looper * shared, Runnable * run) {
        return shared->exists(run);
    }

    void SharedLooperFlush(Looper * shared) {
        shared->flush();
    }
}
