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
#define LOG_NDEBUG 0
#include "Log.h"

#include "stl/List.h"
#include "stl/Queue.h"

#include "System.h"
#include "Mutex.h"
#include "Looper.h"

// https://stackoverflow.com/questions/24854580/how-to-properly-suspend-threads
#include <signal.h>

#include "compat/pthread.h"

#define LOCKFREE_QUEUE  False

__BEGIN_NAMESPACE_ABE

static ABE_INLINE const Char * signame(int signo) {
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
    Int64           mWhen;

    Task() : mWait(Nil), mJob(Nil), mWhen(0) { }
    
    Task(const sp<Job>& job, Int64 delay) : mWait(Nil), mJob(job),
    mWhen(SystemTimeUs() + (delay < 0 ? 0 : delay)) { }

    Bool operator<(const Task& rhs) const {
        return mWhen < rhs.mWhen;
    }
};

struct Stat {
    Int64     start_time;
    Int64     sleep_time;
    Int64     exec_time;
    Int64     last_sleep;
    Int64     last_exec;
    UInt32      num_job;
    UInt32      num_job_late;
    UInt32      num_job_early;
    Int64     job_late_time;
    Int64     job_early_time;
    Bool        profile_enabled;
    Int64     profile_interval;
    Int64     last_profile_time;

    ABE_INLINE Stat() {
        start_time = sleep_time = exec_time = 0;
        num_job = num_job_late = num_job_early = 0;
        job_late_time = job_early_time = 0;
        profile_enabled = False;
    }

    ABE_INLINE void start() {
        start_time = SystemTimeUs();
    }

    ABE_INLINE void stop() {
    }

    ABE_INLINE void profile(Int64 interval) {
        profile_enabled = True;
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
        const Int64 now = SystemTimeUs();
        exec_time += now - last_exec;

        if (profile_enabled && now > last_profile_time + profile_interval) {
            Int64 total_time = now - start_time;
            Float64 usage = 1 - (Float64)sleep_time / total_time;         // wakeup time
            Float64 overhead = usage - (Float64)exec_time / total_time;   // wakeup time - exec time
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
        const Int64 ns = SystemTimeUs() - last_sleep;
        sleep_time += ns;
    }
};

struct JobDispatcher : public Job {
    String                          mName;
    // mutable context, access with lock
    mutable Mutex                   mTaskLock;
#if LOCKFREE_QUEUE
    // check perf test, in multi-producer & single consumer case,
    // LockFree::Queue is much faster than List
    mutable LockFree::Queue<Task>   mTasks;
#endif
    mutable List<Task>              mTimedTasks;
    // with both mTasks & mTimedTasks queues, pop() will be hard,
    // in extreme case, mTasks or mTimedTasks will be ignored forever.
    // so we use an extra virable to handle this situation.
    enum { IMMEDIATE = 0, TIMED = 1 };
    Atomic<int>                     mHacker;

    JobDispatcher(const String& name) : Job(), mName(name), mHacker(TIMED) { }
    
    virtual Bool queue(const sp<Job>& job, Condition* wait) {
        Task task(job, 0);
        task.mWait = wait;
#if LOCKFREE_QUEUE
        mTasks.push(task);
        mHacker.store(IMMEDIATE);
        return mTasks.size() == 1;
#else
        List<Task>::iterator it = mTimedTasks.begin();
        while (it != mTimedTasks.end() && (*it).mWhen < task.mWhen) {
            ++it;
        }

        // insert() will change begin()
        Bool first = it == mTimedTasks.begin();
        mTimedTasks.insert(it, task);
        mHacker.store(TIMED);
        return mTimedTasks.size() == 1;
#endif
    }

    // queue a job, return True if it is the first one
    virtual Bool queue(const sp<Job>& job, Int64 delay = 0) {
        Task task(job, delay);
        
#if LOCKFREE_QUEUE
        // using lockfree queue to speed up queue()
        // but this will cause remove() and exists() more complex
        if (delay <= 0) {
            mTasks.push(task);
            mHacker.store(IMMEDIATE);
            return mTasks.size() == 1;
        }
#endif
        // else push job into mTimedTasks
        AutoLock _l(mTaskLock);

        // go through list should be better than sort()
        List<Task>::iterator it = mTimedTasks.begin();
        while (it != mTimedTasks.end() && (*it).mWhen < task.mWhen) {
            ++it;
        }

        // insert() will change begin()
        Bool first = it == mTimedTasks.begin();
        mTimedTasks.insert(it, task);
        mHacker.store(TIMED);
#if LOCKFREE_QUEUE
        return mTasks.empty() && first;
#else
        return first;
#endif
    }

    // return True when pop success with a job, otherwise set
    // next: set to -1 if no job exists, or next job time in us
    Bool pop(Task& job, Int64 * next) {
        AutoLock _l(mTaskLock);

        *next = -1;     // job not exists
        
#if LOCKFREE_QUEUE
        if (mHacker.load() == IMMEDIATE) {
            if (mTasks.pop(job)) {
                mHacker.store(TIMED);
                next = 0;
                return True;
            }
        }
#endif
        
        if (mTimedTasks.size()) {
            Task& head = mTimedTasks.front();
            const Int64 now = SystemTimeUs();
            // with 1ms jitter:
            // our SleepForInterval and waitRelative based on ns,
            // but os backend implementation can not guarentee it
            // miniseconds precise is the least.
            if (head.mWhen <= now + 1000LL) {
                job = head;
                mTimedTasks.pop();
                next = 0;
                return True;
            }

            *next = head.mWhen - now;
        }

#if LOCKFREE_QUEUE
        if (mTasks.pop(job)) {
            next = 0;
            return True;
        }
#endif
        return False;
    }
    
    // return positive when next exists, otherwise return -1
    Int64 next() const {
        AutoLock _l(mTaskLock);
        if (mTimedTasks.size()) {
            Task& head = mTimedTasks.front();
            const Int64 now = SystemTimeUs();
            if (head.mWhen <= now + 1000LL) {
                return 0;
            }
            return head.mWhen - now;
        }
#if LOCKFREE_QUEUE
        return mTasks.empty() ? -1 : 0;
#else
        return -1;
#endif
    }
    
#if LOCKFREE_QUEUE
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
#endif

    // remove a job, return True if it is at head
    virtual Bool remove(const sp<Job>& job) {
        AutoLock _l(mTaskLock);
#if LOCKFREE_QUEUE
        merge_l();
#endif
        
        Bool head = False;
        List<Task>::iterator it = mTimedTasks.begin();
        while (it != mTimedTasks.end()) {
            if ((*it).mJob == job) {
                it = mTimedTasks.erase(it);
                if (it == mTimedTasks.begin()) head = True;
            } else {
                ++it;
            }
        }

        return head;
    }

    Bool exists(const sp<Job>& job) const {
        AutoLock _l(mTaskLock);
#if LOCKFREE_QUEUE
        merge_l();
#endif

        List<Task>::const_iterator it = mTimedTasks.cbegin();
        for (; it != mTimedTasks.cend(); ++it) {
            if ((*it).mJob == job) {
                return True;
            }
        }
        return False;
    }

    void flush() {
        AutoLock _l(mTaskLock);
        mTimedTasks.clear();
#if LOCKFREE_QUEUE
        mTasks.clear();
#endif
    }
};

struct LooperDispatcher : public JobDispatcher {
    Stat                mStat;      // only used inside dispacher
    const Bool          mMain;
    // internal context
    Looper *            mLooper;    // for Looper::Current()
    
    Mutex               mLock;
    mutable Condition   mWait;
    
    Bool                mTerminated;
    Bool                mRequestExit;
    
    LooperDispatcher(Looper *lp, const String& name, eThreadType type = kThreadDefault) :
    JobDispatcher(name), mMain(False), mLooper(lp), mTerminated(False), mRequestExit(False) {
        CreateThread(type, ThreadEntry, this);
    }
    
    // for main looper
    LooperDispatcher(Looper *lp) : JobDispatcher("main"),
    mMain(True), mLooper(lp), mTerminated(False), mRequestExit(False) {
        CHECK_TRUE(pthread_main(), "main Looper must init in main thread");
        // install signal handlers
        // XXX: make sure main looper can terminate by ctrl-c
        struct sigaction act;
        sigemptyset(&act.sa_mask);
        sigaddset(&act.sa_mask, SIGINT);
        act.sa_flags = SA_RESTART | SA_SIGINFO;
        act.sa_sigaction = sigaction_exit;

        CHECK_EQ(sigaction(SIGINT, &act, Nil), 0);
    }

    static void sigaction_exit(int signum, siginfo_t *info, void *vcontext) {
        INFO("sig %s @ [%d, %d]", signame(info->si_signo), info->si_pid, info->si_uid);
        Looper::Main()->terminate();
        INFO("main: exit...");
    }
    
    ~LooperDispatcher() {
        DEBUG("~LooperDispatcher");
    }
    
    virtual Bool queue(const sp<Job>& job, Int64 us = 0) {
#if 0
        // requestExit will wait for current jobs to complete
        // but no more new jobs
        AutoLock _l(mLock);
        if (mTerminated || mRequestExit) {
            INFO("post after terminated");
            return False;
        }
#endif
        
        if (JobDispatcher::queue(job, us)) {
            AutoLock _l(mLock);
            mWait.signal();
            return True;
        }
        return False;
    }
    
    virtual Bool remove(const sp<Job>& job) {
        if (JobDispatcher::remove(job)) {
            AutoLock _l (mLock);
            mWait.signal();
            return True;
        }
        return False;
    }
    
    void requestExit_l(Bool wait = True) {
        mRequestExit    = True;
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
    }
    
    static void ThreadEntry(void * user) {
        static_cast<LooperDispatcher *>(user)->execution();
        // -> onJob
        DEBUG("EXIT ThreadEntry");
    }
    
    virtual void onJob() {
        mStat.start();

        for (;;) {
            AutoLock _l(mLock);
            Int64 next = 0;
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
        mTerminated = True;
        mWait.broadcast();
        mStat.stop();
    }

    // for main looper only
    virtual void loop() {
        CHECK_TRUE(mMain, "loop() is available for main looper only");
        CHECK_TRUE(pthread_main(), "loop() can only be called in main thread");
        execution();
    }

    virtual void terminate() {
        AutoLock _l(mLock);
        CHECK_TRUE(mMain, "terminate() is available for main looper only");

        // if terminate in main thread, do NOT wait for jobs complete
        requestExit_l(!pthread_main());
        // DO NOT WAIT
    }

    void profile(Int64 interval = 5 * 1000000LL) {
        mStat.profile(interval);
    }
};

//////////////////////////////////////////////////////////////////////////////////
// main looper without backend thread
// auto clear lpMain on last ref release
static Looper * lpMain = Nil;
sp<Looper> Looper::Main() {
    if (lpMain == Nil) {
        INFO("init main looper");
        lpMain = new Looper;
        lpMain->mJobDisp = new LooperDispatcher(lpMain);
    }
    return lpMain;
}

static __thread Looper * lpCurrent = Nil;
struct Initilizer : public Job {
    Looper  *   mLooper;
    String      mName;
    Initilizer() : Job() { }
    virtual void onJob() {
        lpCurrent = mLooper;
        
        // set thread name
        pthread_setname(mName.c_str());
    }
};

sp<Looper> Looper::Current() {
    // no lock need, lpCurrent is a thread context
    return lpCurrent ? lpCurrent : Main();
}

Looper::Looper(const String& name, const eThreadType& type) : SharedObject(FOURCC('loop')),
mJobDisp(new LooperDispatcher(this, name, type)) {
    sp<Initilizer> job = new Initilizer;
    job->mLooper    = this;
    job->mName      = name;
    post(job);
}

void Looper::onFirstRetain() {
    DEBUG("onFirstRetain");
}

void Looper::onLastRetain() {
    DEBUG("onLastRetain");
    if (this == lpMain) {
        INFO("releasing main looper");
        lpMain = Nil;
    } else {
        sp<LooperDispatcher> disp = mJobDisp;
        disp->requestExit();    // request exit without flush
        mJobDisp.clear();
    }
}

void Looper::loop() {
    sp<LooperDispatcher> disp = mJobDisp;
    disp->loop();
}

void Looper::terminate() {
    sp<LooperDispatcher> disp = mJobDisp;
    disp->terminate();
}

void Looper::profile(Int64 interval) {
    sp<LooperDispatcher> disp = mJobDisp;
    disp->profile(interval);
}

void Looper::post(const sp<Job>& job, Int64 delayUs) {
    mJobDisp->queue(job, delayUs);
}

void Looper::remove(const sp<Job>& job) {
    mJobDisp->remove(job);
}

Bool Looper::exists(const sp<Job>& job) const {
    return mJobDisp->exists(job);
}

void Looper::flush() {
    mJobDisp->flush();
}

//////////////////////////////////////////////////////////////////////////////////
static Atomic<Int64> QueueID = 0;
static String MakeQueueName() {
    return String::format("queue-%" PRId64, QueueID++);
}

struct QueueDispatcher : public JobDispatcher {
    Mutex           mLock;
    Condition       mWait;
    Atomic<UInt32>  mSched; // +1 every time post dispatcher into looper
    
    QueueDispatcher() : JobDispatcher(MakeQueueName()), mSched(0) {
        
    }
    
    // no wait() or sleep or loop in onJob,
    // or it will block underlying looper
    virtual void onJob() {
        AutoLock _l(mLock);
        --mSched;
        Task job;
        Int64 next = 0;
        // execute current job
        if (pop(job, &next)) {
            mLock.unlock();
            job.mJob->execution();
            mLock.lock();
            if (job.mWait) {
                job.mWait->signal();    // for sync job
            }
            next = JobDispatcher::next();
        } else {
            DEBUG("dispatch without job ready, next = %.3f(s)", next / 1E6);
        }
        
        // no more jobs
        if (next < 0) {
            // exit on jobs finished
            DEBUG("%s: no more jobs", mName.c_str());
            mWait.signal();
            return;
        }
        
        if (mSched.load() == 0) {
            // NO more dispatcher in looper, queue a new one
            Looper::Current()->post(this, next);
            ++mSched;
        }
    }
    
    // request exit and wait.
    void requestExit() {
        AutoLock _l(mLock);
        // NOT scheduled & no more jobs
        if (mSched.load() == 0 && JobDispatcher::next() < 0) {
            DEBUG("%s: not dispatching...next %.3f(s)", mName.c_str());
            return;
        }
        
        DEBUG("%s: wait for dispatcher complete", mName.c_str());
        mWait.wait(mLock);
    }
};

DispatchQueue::DispatchQueue(const sp<Looper>& lp) :
mLooper(lp), mDispatcher(new QueueDispatcher()) {
}

DispatchQueue::~DispatchQueue() {
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
        ++disp->mSched;
    }
    wait.wait(disp->mLock);
}

void DispatchQueue::dispatch(const sp<Job>& job, Int64 us) {
    sp<QueueDispatcher> disp = mDispatcher;
    AutoLock _l(disp->mLock);
    DEBUG("dispatch job @ %.3f(s)", us / 1E6);
    if (mDispatcher->queue(job, us)) {
        DEBUG("schedule @ %.3f(s)", us / 1E6);
        // remove will affect dispatch's efficiency.
        mLooper->post(mDispatcher, us);
        ++disp->mSched;
    }
}

Bool DispatchQueue::exists(const sp<Job>& job) const {
    sp<QueueDispatcher> disp = mDispatcher;
    //AutoLock _l(disp->mLock);
    return mDispatcher->exists(job);
}

void DispatchQueue::remove(const sp<Job>& job) {
    sp<QueueDispatcher> disp = mDispatcher;
    //AutoLock _l(disp->mLock);
    if (mDispatcher->remove(job)) {
        mLooper->post(mDispatcher);
        ++disp->mSched;
    }
}

void DispatchQueue::flush() {
    sp<QueueDispatcher> disp = mDispatcher;
    //AutoLock _l(disp->mLock);
    mDispatcher->flush();
    // don't remove dispatcher here, let it terminate automatically
}

__END_NAMESPACE_ABE
