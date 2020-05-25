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

#define _DARWIN_C_SOURCE    // sys_signame

#define LOG_TAG   "Looper"
//#define LOG_NDEBUG 0
#include "Log.h"

#include "stl/List.h"
#include "stl/Vector.h"
#include "stl/Queue.h"

#include "Time.h"
#include "Mutex.h"
#include "Looper.h"

// https://stackoverflow.com/questions/24854580/how-to-properly-suspend-threads
#include <signal.h>

#include "compat/pthread.h"

__BEGIN_NAMESPACE_ABE

static ABE_INLINE const char * signame(int signo) {
#ifdef __APPLE__
    return sys_signame[signo];
#elif defined(_WIN32) || defined(__MINGW32__)
    switch (signo) {
        case SIGINT:    return "SIGINT";
        case SIGILL:    return "SIGILL";
        case SIGFPE:    return "SIGFPE";
        case SIGSEGV:   return "SIGSEGV";
        case SIGTERM:   return "SIGTERM";
        case SIGABRT:   return "SIGABRT";
        case SIGBREAK:  return "SIGBREAK";
        default:        return "Unknown";
    }
#else
    return strsignal(signo);
#endif
}

static __thread Looper * lpCurrent = NULL;
static Looper * lpMain = NULL;

struct Task {
    Object<Job>     mJob;
    int64_t         mWhen;

    Task() : mJob(NULL), mWhen(0) { }
    Task(const Object<Job>& job, int64_t delay) : mJob(job),
    mWhen(SystemTimeUs() + (delay < 0 ? 0 : delay)) { }

    bool operator<(const Task& rhs) const {
        return mWhen < rhs.mWhen;
    }
};

struct Stat {
    int64_t     start_time;
    int64_t     sleep_time;
    int64_t     exec_time;
    int64_t     last_sleep;
    int64_t     last_exec;
    size_t      num_job;
    size_t      num_job_late;
    size_t      num_job_early;
    int64_t     job_late_time;
    int64_t     job_early_time;
    bool        profile_enabled;
    int64_t     profile_interval;
    int64_t     last_profile_time;

    ABE_INLINE Stat() {
        start_time = sleep_time = exec_time = 0;
        num_job = num_job_late = num_job_early = 0;
        job_late_time = job_early_time = 0;
        profile_enabled = false;
    }

    ABE_INLINE void start() {
        start_time = SystemTimeUs();
    }

    ABE_INLINE void stop() {
    }

    ABE_INLINE void profile(int64_t interval) {
        profile_enabled = true;
        profile_interval = interval;
        last_profile_time = SystemTimeUs();
    }

    ABE_INLINE void start_profile(const Task& job) {
        ++num_job;
        last_exec = SystemTimeUs();
        if (job.mWhen < last_exec) {
            ++num_job_late;
            job_late_time += (last_exec - job.mWhen);
        } else {
            ++num_job_early;
            job_early_time += (job.mWhen - last_exec);
        }
    }

    ABE_INLINE void end_profile(const Task& job) {
        const int64_t now = SystemTimeUs();
        exec_time += now - last_exec;

        if (profile_enabled && now > last_profile_time + profile_interval) {
            int64_t total_time = now - start_time;
            double usage = 1 - (double)sleep_time / total_time;         // wakeup time
            double overhead = usage - (double)exec_time / total_time;   // wakeup time - exec time
            INFO("looper: %zu jobs, usage %.2f%%, overhead %.2f%%, each job %" PRId64 " us, late by %" PRId64 " us",
                    num_job, 100 * usage, 100 * overhead,
                    exec_time / num_job, job_late_time / num_job);

            last_profile_time = now;
        }
    }

    ABE_INLINE void sleep() {
        last_sleep = SystemTimeUs();
    }

    ABE_INLINE void wakeup() {
        const int64_t ns = SystemTimeUs() - last_sleep;
        sleep_time += ns;
    }
};


struct Looper::JobDispatcher : public Job {
    // internal context
    Stat                            mStat;  // only used inside looper

    // mutable context, access with lock
    mutable Mutex                   mLock;
    mutable Condition               mWait;
    mutable LockFree::Queue<Task>   mTasks;
    mutable List<Task>              mTimedTasks;
    Vector<void *>                  mUserContext;

    Looper *                        mLooper;
    Object<Thread>                  mThread;
    bool                            mTerminated;
    bool                            mRequestExit;
    bool                            mIsMain;
    String                          mName;
    eThreadType                     mType;

    JobDispatcher(Looper *lp, const String& name, eThreadType type) : 
        Job(), mLooper(lp), mTerminated(false), mRequestExit(false), 
        mIsMain(false), mName(name), mType(type) { }

    void init(bool main = false) {
        if (mThread.isNIL()) {
            if (mIsMain) {
                CHECK_TRUE(pthread_main(), "main Looper must init in main thread");
                pthread_setname("main");
                // install signal handlers
                // XXX: make sure main looper can terminate by ctrl-c
                struct sigaction act;
                sigemptyset(&act.sa_mask);
                sigaddset(&act.sa_mask, SIGINT);
                act.sa_flags = SA_RESTART | SA_SIGINFO;
                act.sa_sigaction = sigaction_exit;

                CHECK_EQ(sigaction(SIGINT, &act, NULL), 0);
            } else {
                mThread = new Thread(this);
                mThread->setName(mName).setType(mType).run();
            }
        }
    }

    void profile(int64_t interval = 5 * 1000000LL) {
        mStat.profile(interval);
    }

    void post(const Object<Job>& job, int64_t delay = 0) {
        Task task(job, delay);
        
        // requestExit will wait for current jobs to complete
        // but no more new jobs
        if (mTerminated || mRequestExit) {
            INFO("post after terminated");
            return;
        }

        // using lockfree queue to speed up post()
        // but this will cause remove() and exists() more complex
        if (delay == 0) {
            mTasks.push(task);
            mWait.signal();
            return;
        }

        // else push job into mTimedTasks
        AutoLock _l(mLock);

        // go through list should be better than sort()
        List<Task>::iterator it = mTimedTasks.begin();
        while (it != mTimedTasks.end() && (*it).mWhen < task.mWhen) {
            ++it;
        }

        if (it == mTimedTasks.begin()) {
            // have signal before insert(), as insert() will change begin()
            mWait.signal();
        }
        mTimedTasks.insert(it, task);
    }

    // return reference to job if job is ready, or return NULL
    // next: set to -1 if not job exists, or next job time in us
    bool pop(Task& job, int64_t * next) {
        AutoLock _l(mLock);

        *next = -1;     // job not exists

        if (mTimedTasks.size()) {
            Task& head = mTimedTasks.front();
            const int64_t now = SystemTimeUs();
            // with 1ms jitter:
            // our SleepForInterval and waitRelative based on ns,
            // but os backend implementation can not guarentee it
            // miniseconds precise is the least.
            if (head.mWhen <= now + 1000LL) {
                job = head;
                mTimedTasks.pop();
                return true;
            }

            *next = head.mWhen - now;
        }

        if (mTasks.pop(job)) {
            return true;
        } else {
            return false;
        }
    }

    void remove(const Object<Job>& job) {
        AutoLock _l(mLock);

        // move mTasks -> mTimedTasks
        if (mTasks.size()) {
            Task tmp;
            while (mTasks.pop(tmp)) {
                mTimedTasks.push(tmp);
            }
            mTimedTasks.sort();
        }

        bool head = false;
        List<Task>::iterator it = mTimedTasks.begin();
        while (it != mTimedTasks.end()) {
            if ((*it).mJob == job) {
                it = mTimedTasks.erase(it);
                if (it == mTimedTasks.begin()) head = true;
            } else {
                ++it;
            }
        }

        if (head) mWait.signal();
    }

    bool exists(const Object<Job>& job) const {
        AutoLock _l(mLock);

        // move mTasks -> mTimedTasks
        if (mTasks.size()) {
            Task tmp;
            while (mTasks.pop(tmp)) {
                mTimedTasks.push(tmp);
            }
            mTimedTasks.sort();
        }

        List<Task>::const_iterator it = mTimedTasks.cbegin();
        for (; it != mTimedTasks.cend(); ++it) {
            if ((*it).mJob == job) {
                return true;
            }
        }
        return false;
    }

    void flush() {
        AutoLock _l(mLock);
        flush_l();
    }

    void flush_l() {
        mTimedTasks.clear();
        mTasks.clear();
        mWait.signal();
    }

    void requestExit_l(bool wait = true) {
        mRequestExit    = true;
        if (wait) mWait.signal();
        else flush_l();
    }

    virtual void requestExit() {
        AutoLock _l(mLock);
        if (mTerminated) return;

        requestExit_l();
        
        if (mThread.isNIL()) return;
        
        mWait.broadcast();
        while (!mTerminated) mWait.wait(mLock);

        mLock.unlock();
        mThread->join();
        mLock.lock();
    }

    virtual void onJob() {
        lpCurrent = this->mLooper;

        mStat.start();

        for (;;) {
            int64_t next = 0;
            Task job;
            if (pop(job, &next)) {
                mStat.start_profile(job);
                job.mJob->onJobInt();
                mStat.end_profile(job);
                continue;
            }

            AutoLock _l(mLock);
            if (next > 0) {
                mStat.sleep();
                mWait.waitRelative(mLock, next * 1000);
                mStat.wakeup();
            } else if (next < 0) {
                // no more jobs
                if (mRequestExit) {
                    DEBUG("exiting...");
                    break;
                }
                mStat.sleep();
                mWait.wait(mLock);
                mStat.wakeup();
            }
        }

        AutoLock _l(mLock);
        mTerminated = true;
        mWait.broadcast();
        mStat.stop();
        if (lpCurrent == lpMain) lpMain = NULL;
        lpCurrent = NULL;
    }

    static void sigaction_exit(int signum, siginfo_t *info, void *vcontext) {
        INFO("sig %s @ [%d, %d]", signame(info->si_signo), info->si_pid, info->si_uid);
        lpMain->mJobDisp->terminate();
        INFO("main: exit...");
    }

    // for main looper only
    virtual void loop() {
        onJobInt();
    }

    virtual void terminate() {
        AutoLock _l(mLock);
        CHECK_TRUE(mIsMain, "terminate() is available for main looper only");
        
        // wait for looper finish
        if (pthread_main()) {
            // terminate in main thread
            // DO NOT wait for jobs complete
            requestExit_l(false);
            return;
        } else {
            requestExit_l(true);
            mWait.broadcast();
            while (!mTerminated) mWait.wait(mLock);
        }
    }
};

//////////////////////////////////////////////////////////////////////////////////
Object<Looper> Looper::Current() {
    // no lock need, lpCurrent is a thread context
    return lpCurrent ? lpCurrent : Main();
}

// main looper without backend thread
// auto clear __main on last ref release
Object<Looper> Looper::Main() {
    if (lpMain == NULL) {   // it is ok without lock
        DEBUG("init main looper");
        lpMain = new Looper;
        lpMain->mJobDisp->mIsMain = true;
    }
    return lpMain;
}

// always keep a ref to global looper
// client have to clear it by set global looper to NULL
static Object<Looper> __global = NULL;
Object<Looper> Looper::Global() { return __global; }
void Looper::SetGlobal(const Object<Looper>& lp) { __global = lp; }

static Atomic<uint32_t> lpID = 0u;
static String MakeLooperName() {
    return String::format("looper-%" PRIu32, lpID++);
}

Looper::Looper() : SharedObject(OBJECT_ID_LOOPER), 
    mJobDisp(new JobDispatcher(this, MakeLooperName(), kThreadDefault)) { }

Looper::Looper(const String& name, const eThreadType& type) : SharedObject(OBJECT_ID_LOOPER), 
    mJobDisp(new JobDispatcher(this, name, type)) { }

void Looper::onFirstRetain() {
    mJobDisp->init();
}

void Looper::onLastRetain() {
    mJobDisp->requestExit();    // request exit without flush
    mJobDisp.clear();
}

void Looper::loop() {
    CHECK_TRUE(pthread_main(), "loop() can only be called in main thread");
    mJobDisp->loop();
}

void Looper::terminate() {
    mJobDisp->terminate();
}

Thread& Looper::thread() {
    return mJobDisp->mThread.isNIL() ? *mJobDisp->mThread : Thread::Main();
}

void Looper::profile(int64_t interval) {
    mJobDisp->profile(interval);
}

void Looper::post(const Object<Job>& job, int64_t delayUs) {
    mJobDisp->post(job, delayUs);
}

void Looper::remove(const Object<Job>& job) {
    mJobDisp->remove(job);
}

bool Looper::exists(const Object<Job>& job) const {
    return mJobDisp->exists(job);
}

void Looper::flush() {
    mJobDisp->flush();
}

String Looper::string() const {
    String info;
#if 0
    String info = String::format("looper %s: jobs %zu",
            looper->mDispatcher->mName.c_str(),
            looper->mDispatcher->mTasks.size());

    size_t index = 0;
    List<Task>::const_iterator it = shared->mTasks.cbegin();
    for (; it != shared->mTasks.cend(); ++it) {
        const Task& job = *it;
        info.append("\n");
        info.append(String::format("Task [%zu]: ***", index++));
    }
#endif
    return info;
}

size_t Looper::bind(void * user) {
    AutoLock _l(mJobDisp->mLock);
    mJobDisp->mUserContext.push(user);
    return mJobDisp->mUserContext.size() - 1;
}

void Looper::bind(size_t id, void *user) {
    AutoLock _l(mJobDisp->mLock);
    CHECK_LE(id, mJobDisp->mUserContext.size());
    if (id == mJobDisp->mUserContext.size()) {
        mJobDisp->mUserContext.push(user);
    } else {
        mJobDisp->mUserContext[id] = user;
    }
}

void * Looper::user(size_t id) const {
    AutoLock _l(mJobDisp->mLock);
    if (id >= mJobDisp->mUserContext.size())
        return NULL;
    return mJobDisp->mUserContext[id];
}

__END_NAMESPACE_ABE
