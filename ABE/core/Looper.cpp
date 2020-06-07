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
#include "stl/Queue.h"

#include "System.h"
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

struct Task {
    Condition *     mWait;
    sp<Job>         mJob;
    int64_t         mWhen;

    Task() : mWait(NULL), mJob(NULL), mWhen(0) { }
    
    Task(const sp<Job>& job, int64_t delay) : mWait(NULL), mJob(job),
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

struct JobDispatcher : public Job {
    String                          mName;
    // mutable context, access with lock
    mutable Mutex                   mTaskLock;
    mutable LockFree::Queue<Task>   mTasks;
    mutable List<Task>              mTimedTasks;

    JobDispatcher(const String& name) :
        Job(), mName(name) { }
    
    virtual bool queue(const sp<Job>& job, Condition* wait) {
        Task task(job, 0);
        task.mWait = wait;
        mTasks.push(task);
        return mTasks.size() == 1;
    }

    // queue a job, return true if it is the first one
    virtual bool queue(const sp<Job>& job, int64_t delay = 0) {
        Task task(job, delay);
        
        // using lockfree queue to speed up queue()
        // but this will cause remove() and exists() more complex
        if (delay == 0) {
            mTasks.push(task);
            return mTasks.size() == 1;
        }

        // else push job into mTimedTasks
        AutoLock _l(mTaskLock);

        // go through list should be better than sort()
        List<Task>::iterator it = mTimedTasks.begin();
        while (it != mTimedTasks.end() && (*it).mWhen < task.mWhen) {
            ++it;
        }

        // insert() will change begin()
        bool first = it == mTimedTasks.begin();
        mTimedTasks.insert(it, task);
        return mTasks.empty() && first;
    }

    // return true when pop success with a job, otherwise set
    // next: set to -1 if no job exists, or next job time in us
    bool pop(Task& job, int64_t * next) {
        AutoLock _l(mTaskLock);

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
                next = 0;
                return true;
            }

            *next = head.mWhen - now;
        }

        if (mTasks.pop(job)) {
            next = 0;
            return true;
        } else {
            return false;
        }
    }
    
    // return positive when next exists, otherwise return -1
    int64_t next() const {
        AutoLock _l(mTaskLock);
        if (mTimedTasks.size()) {
            Task& head = mTimedTasks.front();
            const int64_t now = SystemTimeUs();
            if (head.mWhen <= now + 1000LL) {
                return 0;
            }
            return head.mWhen - now;
        }
        return mTasks.empty() ? -1 : 0;
    }
    
    ABE_INLINE void merge_l() const {
        // move mTasks -> mTimedTasks
        if (mTasks.size()) {
            Task tmp;
            while (mTasks.pop(tmp)) {
                mTimedTasks.push_front(tmp);
            }
            mTimedTasks.sort();
        }
    }

    // remove a job, return true if it is at head
    virtual bool remove(const sp<Job>& job) {
        AutoLock _l(mTaskLock);
        merge_l();
        
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

        return head;
    }

    bool exists(const sp<Job>& job) const {
        AutoLock _l(mTaskLock);
        merge_l();

        List<Task>::const_iterator it = mTimedTasks.cbegin();
        for (; it != mTimedTasks.cend(); ++it) {
            if ((*it).mJob == job) {
                return true;
            }
        }
        return false;
    }

    void flush() {
        AutoLock _l(mTaskLock);
        mTimedTasks.clear();
        mTasks.clear();
    }
};

static __thread Looper * lpCurrent = NULL;
static Looper * lpMain = NULL;
struct LooperDispatcher : public JobDispatcher {
    Stat                            mStat;  // only used inside dispacher
    // internal context
    Thread                          mThread;
    Looper *                        mLooper;
    
    Mutex                           mLock;
    mutable Condition               mWait;
    
    bool                            mTerminated;
    bool                            mRequestExit;

    LooperDispatcher(Looper *lp, const String& name, eThreadType type = kThreadDefault) :
        JobDispatcher(name), mThread(this, type),
        mLooper(lp), mTerminated(false), mRequestExit(false) {
            mThread.setName(mName).run();
        }
    
    // for main looper
    LooperDispatcher(Looper *lp) : JobDispatcher("main"), mThread(Thread::Main()),
    mLooper(lp), mTerminated(false), mRequestExit(false) {
        CHECK_TRUE(pthread_main(), "main Looper must init in main thread");
        // install signal handlers
        // XXX: make sure main looper can terminate by ctrl-c
        struct sigaction act;
        sigemptyset(&act.sa_mask);
        sigaddset(&act.sa_mask, SIGINT);
        act.sa_flags = SA_RESTART | SA_SIGINFO;
        act.sa_sigaction = sigaction_exit;

        CHECK_EQ(sigaction(SIGINT, &act, NULL), 0);
    }

    static void sigaction_exit(int signum, siginfo_t *info, void *vcontext) {
        INFO("sig %s @ [%d, %d]", signame(info->si_signo), info->si_pid, info->si_uid);
        lpMain->terminate();
        INFO("main: exit...");
    }
    
    virtual bool queue(const sp<Job>& job, int64_t us = 0) {
#if 0
        // requestExit will wait for current jobs to complete
        // but no more new jobs
        AutoLock _l(mLock);
        if (mTerminated || mRequestExit) {
            INFO("post after terminated");
            return false;
        }
#endif
        
        if (JobDispatcher::queue(job, us)) {
            AutoLock _l(mLock);
            mWait.signal();
            return true;
        }
        return false;
    }
    
    virtual bool remove(const sp<Job>& job) {
        if (JobDispatcher::remove(job)) {
            AutoLock _l (mLock);
            mWait.signal();
            return true;
        }
        return false;
    }
    
    void requestExit_l(bool wait = true) {
        mRequestExit    = true;
        if (!wait) flush();
        mWait.signal();
    }

    // request exist and wait
    virtual void requestExit() {
        AutoLock _l(mLock);
        if (mTerminated) return;

        requestExit_l();

        mWait.broadcast();
        
        // wait until job dispatcher terminated
        while (!mTerminated) mWait.wait(mLock);
        
        if (mThread != Thread::Main()) {
            mThread.join();
        }
    }
    
    virtual void onJob() {
        lpCurrent = this->mLooper;

        mStat.start();

        for (;;) {
            AutoLock _l(mLock);
            int64_t next = 0;
            Task job;
            if (pop(job, &next)) {
                mStat.start_profile(job);
                mLock.unlock();
                job.mJob->execution();
                mLock.lock();
                mStat.end_profile(job);
                continue;
            }

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

    // for main looper only
    virtual void loop() {
        CHECK_TRUE(mThread == Thread::Main(), "loop() is available for main looper only");
        execution();
    }

    virtual void terminate() {
        AutoLock _l(mLock);
        CHECK_TRUE(mThread == Thread::Main(), "terminate() is available for main looper only");

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

    void profile(int64_t interval = 5 * 1000000LL) {
        mStat.profile(interval);
    }
};

//////////////////////////////////////////////////////////////////////////////////
// main looper without backend thread
// auto clear __main on last ref release
sp<Looper> Looper::Main() {
    if (lpMain == NULL) {
        INFO("init main looper");
        lpMain = new Looper;
        lpMain->mJobDisp = new LooperDispatcher(lpMain);
    }
    return lpMain;
}

sp<Looper> Looper::Current() {
    // no lock need, lpCurrent is a thread context
    return lpCurrent ? lpCurrent : Main();
}

Looper::Looper(const String& name, const eThreadType& type) : SharedObject(OBJECT_ID_LOOPER), 
    mJobDisp(new LooperDispatcher(this, name, type)) {
    }

void Looper::onFirstRetain() {
}

void Looper::onLastRetain() {
    sp<LooperDispatcher> disp = mJobDisp;
    disp->requestExit();    // request exit without flush
    mJobDisp.clear();
}

void Looper::loop() {
    CHECK_TRUE(pthread_main(), "loop() can only be called in main thread");
    sp<LooperDispatcher> disp = mJobDisp;
    disp->loop();
}

void Looper::terminate() {
    sp<LooperDispatcher> disp = mJobDisp;
    disp->terminate();
}

Thread& Looper::thread() {
    sp<LooperDispatcher> disp = mJobDisp;
    return disp->mThread;
}

void Looper::profile(int64_t interval) {
    sp<LooperDispatcher> disp = mJobDisp;
    disp->profile(interval);
}

void Looper::post(const sp<Job>& job, int64_t delayUs) {
    mJobDisp->queue(job, delayUs);
}

void Looper::remove(const sp<Job>& job) {
    mJobDisp->remove(job);
}

bool Looper::exists(const sp<Job>& job) const {
    return mJobDisp->exists(job);
}

void Looper::flush() {
    mJobDisp->flush();
}

//////////////////////////////////////////////////////////////////////////////////
static Atomic<int64_t> QueueID = 0;
static String MakeQueueName() {
    return String::format("queue-%" PRId64, QueueID++);
}

struct QueueDispatcher : public JobDispatcher {
    Mutex       mLock;
    Condition   mWait;
    bool        mDispatching;
    
    QueueDispatcher() : JobDispatcher(MakeQueueName()), mDispatching(false) {
        
    }
    
    // no wait() or sleep or loop in onJob,
    // or it will block underlying looper
    virtual void onJob() {
        AutoLock _l(mLock);
        mDispatching = true;
        Task job;
        int64_t next = 0;
        // execute current job
        if (pop(job, &next)) {
            mLock.unlock();
            job.mJob->execution();
            mLock.lock();
            if (job.mWait) {
                job.mWait->signal();    // for sync job
            }
            next = JobDispatcher::next();
        }
        
        // no more jobs
        if (next < 0) {
            // exit on jobs finished
            DEBUG("%s: no more jobs", mName.c_str());
            mDispatching = false;
            mWait.signal();
            return;
        }
        
        Looper::Current()->post(this, next);
    }
    
    // request exit and wait.
    void requestExit() {
        AutoLock _l(mLock);
        // no dispatching & no more jobs
        if (mDispatching == false && JobDispatcher::next() < 0) {
            DEBUG("%s: not dispatching...", mName.c_str());
            return;
        }
        
        DEBUG("%s: wait for dispatcher complete", mName.c_str());
        mWait.wait(mLock);
        CHECK_FALSE(mDispatching);
    }
};

DispatchQueue::DispatchQueue(const sp<Looper>& lp) :
mLooper(lp), mDispatcher(new QueueDispatcher()) {
}

DispatchQueue::~DispatchQueue() {
    mLooper->remove(mDispatcher);
}

void DispatchQueue::onFirstRetain() {
    
}

void DispatchQueue::onLastRetain() {
    sp<QueueDispatcher> disp = mDispatcher;
    disp->requestExit();
}

void DispatchQueue::sync(const sp<Job>& job) {
    sp<QueueDispatcher> disp = mDispatcher;
    AutoLock _(disp->mLock);
    Condition wait;
    if (disp->queue(job, &wait)) {
        mLooper->post(disp);
    }
    wait.wait(disp->mLock);
}

void DispatchQueue::dispatch(const sp<Job>& job, int64_t us) {
    sp<QueueDispatcher> disp = mDispatcher;
    //AutoLock _l(disp->mLock);
    if (mDispatcher->queue(job, us)) {
        mLooper->post(mDispatcher);
    }
}

bool DispatchQueue::exists(const sp<Job>& job) const {
    sp<QueueDispatcher> disp = mDispatcher;
    //AutoLock _l(disp->mLock);
    return mDispatcher->exists(job);
}

void DispatchQueue::remove(const sp<Job>& job) {
    sp<QueueDispatcher> disp = mDispatcher;
    //AutoLock _l(disp->mLock);
    if (mDispatcher->remove(job)) {
        // re-schedule
        mLooper->remove(mDispatcher);
        mLooper->post(mDispatcher);
    }
}

void DispatchQueue::flush() {
    sp<QueueDispatcher> disp = mDispatcher;
    //AutoLock _l(disp->mLock);
    mDispatcher->flush();
    // don't remove dispatcher here, let it terminate automatically
}

__END_NAMESPACE_ABE
