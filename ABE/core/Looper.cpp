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

// XXX: something is wrong with LockFree::Queue,
// fix it first
#define LOCKFREE_QUEUE  False

/**
 * Multiple thread program basis:
 * 1. a job object
 * 2. a looper object backed by thread
 * 3. a dispatch queue object.
 *
 * Job:
 *  A small section of code that can be executed by Looper or DispatchQueue
 *
 * Looper:
 *  A job delegate backed by ThreadScheduler which will re released when all
 *  refs are gone, even if there are jobs in queue.
 *
 * DispatchQueue:
 *  A job delegate backed by QueueScheduler which will be released when all
 *  refs are gone, even if there are jobs in queue.
 *
 * WHY WE MUST DROP JOBS IN QUEUE WHEN EXITING?
 * both Looper & DispatchQueue can be shared by multiple user, keeping execute
 * jobs when all reference gone is MEANINGLESS as all user context are gone.
 * If we wait for jobs to be completed on last refs, then it will block a random user context.
 */
__BEGIN_NAMESPACE_ABE

static ABE_INLINE const Char * signame(Int signo) {
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
    Time            mWhen;

    Task() : mWait(Nil), mJob(Nil) { }

    Task(const sp<Job>& job, Time after) : mWait(Nil), mJob(job),
    mWhen(Time::Now() + after) { }
    
    Task(const sp<Job>& job, Condition * wait) : mWait(wait), mJob(job),
    mWhen(Time::Now()) { }

    Bool operator<(const Task& rhs) const {
        return mWhen < rhs.mWhen;
    }
};
template <> struct is_trivial_move<Task> { enum { value = is_trivial_move<sp<Job> >::value }; };

// JobDelegate has multiple producers but only one consumer.
// JobDelegate is thread safe.
struct JobDelegate : public SharedObject {
    const String                    mName;
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
    Atomic<Int>                     mHacker;

    JobDelegate(const String& name, UInt id) : SharedObject(id),
    mName(name), mHacker(TIMED) { }
    
    virtual ~JobDelegate() {
        if (mTimedTasks.size()) {
            INFO("%s: exit with %" PRIu32 " jobs in queue",
                 mName.c_str(), mTimedTasks.size());
        }
    }

    // notify scheduler
    virtual void notify() = 0;
    
    // execute a job
    ABE_INLINE void work(Task& task) {
        mTaskLock.unlock();
        task.mJob->execution();
        mTaskLock.lock();
        if (task.mWait) {
            task.mWait->signal();
        }
    }
    
    // execute a job and set next job time.
    // return True on success
    // return False on overrun or no job exists.
    // set next to -1 if no job exists.
    Bool dispatch(Int64& next) {
        AutoLock _l(mTaskLock);
        Task task;
        Bool success = False;
#if LOCKFREE_QUEUE
        if (mHacker.load() == IMMEDIATE) {
            if (mTasks.pop(task)) {
                work(task);
                mHacker.store(TIMED);
                success = True;
            }
        }
#endif
        if (!success) {
            if (mTimedTasks.size()) {
                Task& head = mTimedTasks.front();
                // with 1ms jitter:
                // our SleepForInterval and waitRelative based on ns,
                // but os backend implementation can not guarentee it
                // miniseconds precise is the least.
                if (head.mWhen <= Time::Now() + Time::MilliSeconds(1)) {
                    task = head;
                    mTimedTasks.pop();
                    work(task);
                    mHacker.store(IMMEDIATE);
                    success = True;
                }
            }
#if LOCKFREE_QUEUE
            else if (mTasks.pop(task)) {
                work(task);
                success = True;
            }
#endif
        }

        // find next
#if LOCKFREE_QUEUE
        if (mTasks.size()) next = 0;
        else
#endif
        if (mTimedTasks.size()) {
            const Task& head = mTimedTasks.front();
            next = (head.mWhen - Time::Now()).nseconds();
            if (next < 0) next = 0; // fix underrun.
        }
        else next = -1;
        DEBUG("%s: dispatch status %d, next %" PRId64,
              mName.c_str(), success, next);
        return success;
    }
    
    // queue a job and run synchronized.
    // return True on success
    // return False when job be canceled or interrupted.
    Bool sync(const sp<Job>& job, Time deadline = 0) {
        AutoLock _l(mTaskLock);
        Condition wait;
        Task task(job, &wait);
#if LOCKFREE_QUEUE
        mTasks.push(task);
        mHacker.store(IMMEDIATE);
        if (mTasks.size() == 1) {
            notify();
        }
#else
        mTimedTasks.insert(mTimedTasks.begin(), task);
        notify();
#endif
        if (deadline == 0) {
            wait.wait(mTaskLock);
            return True;
        } else {
            return !wait.waitRelative(mTaskLock, deadline);
        }
    }

    // queue a job and run asynchronized.
    void queue(const sp<Job>& job, Time after = 0) {
        Task task(job, after);

#if LOCKFREE_QUEUE
        // using lockfree queue to speed up queue()
        // but this will cause remove() and exists() more complex
        if (after == 0) {
            UInt32 length = mTasks.push(task);
            mHacker.store(IMMEDIATE);
            if (length == 1) notify();
            return;
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
        if (first) notify();
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

    // return True if job exists and removed success.
    Bool remove(const sp<Job>& job) {
        AutoLock _l(mTaskLock);
#if LOCKFREE_QUEUE
        merge_l();
#endif

        Bool success = False;
        Bool head = False;
        List<Task>::iterator it = mTimedTasks.begin();
        while (it != mTimedTasks.end()) {
            if ((*it).mJob == job) {
                it = mTimedTasks.erase(it);
                success = True;
                if (it == mTimedTasks.begin()) head = True;
            } else {
                ++it;
            }
        }
        
        if (head) {
            notify();
        }

        return success;
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
        notify();
    }
};

// scheduler object backed by a thread.
// scheduler may be shared by multiple clients, when
// all clients are gone, the scheduler MUST terminate
// self and the underlying thread.
struct ThreadScheduler : public JobDelegate {
    const eThreadType       mType;
    const Bool              mMain;
    mutable Atomic<UInt32>  mQueueID;   // For QueueScheduler
    // internal context
    Mutex                   mLock;
    mutable Condition       mWait;
    pthread_t               mThread;
    Bool                    mExitPending;
    Bool                    mTerminated;

    ThreadScheduler(const String& name, eThreadType type = kThreadDefault) :
    JobDelegate(name, FOURCC('!lop')),
    mType(type), mMain(False), mExitPending(False), mTerminated(False) {
        CreateThread(mName.c_str(), mType, ThreadEntry, this);
    }

    // for main looper
    ThreadScheduler() : JobDelegate("main", FOURCC('MAIN')), mType(kThreadDefault), mMain(True), mExitPending(False) {
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

    static void sigaction_exit(Int signum, siginfo_t *info, void *vcontext) {
        INFO("sig %s @ [%d, %d]", signame(info->si_signo), info->si_pid, info->si_uid);
        Looper::Main()->terminate();
        INFO("main: exit...");
    }

    ~ThreadScheduler() {
        DEBUG("%s: ~ThreadScheduler", mName.c_str());
    }

    virtual void notify() {
        DEBUG("%s: notify...", mName.c_str());
        AutoLock _l(mLock);
        mWait.signal();
    }

    static void ThreadEntry(void * user) {
        static_cast<ThreadScheduler *>(user)->onSchedule();
    }
        
    virtual void onSchedule() {
        AutoLock _l(mLock);
        DEBUG("%s: onJob enter", mName.c_str());
        // set underlying thread properties.
        mThread = pthread_self();
        
        // scheduler may NOT be shared yet
        if (IsObjectNotShared()) {
            mWait.wait(mLock);
        }
        
        // loop until all reference are gone except the kept one.
        while(!mExitPending) {
            Int64 next;
            
            // unlock when dispatch jobs.
            mLock.unlock();
            dispatch(next);
            mLock.lock();
            
            // MUST test here again.
            if (mExitPending) break;
            
            if (next > 0) {
                DEBUG("%s: overrun", mName.c_str());
                mWait.waitRelative(mLock, next);
                DEBUG("%s: interrupt", mName.c_str());
            } else if (next < 0) {
                DEBUG("%s: sleep", mName.c_str());
                mWait.wait(mLock);
                DEBUG("%s: wakeup", mName.c_str());
            }
        }
        
        DEBUG("%s: goodbye...", mName.c_str());
        mTerminated = True;
        mWait.broadcast();
    }
    
    virtual void onFirstRetain() {
        notify();
    }
    
    virtual void onLastRetain() {
        if (mMain) return;
        AutoLock _l(mLock);
        // this is the last refs, terminate underlying thread.
        mExitPending = True;
        // wake up thread and wait it to exit.
        mWait.signal();
        while (!mTerminated) {
            mWait.wait(mLock);
        }
    }

    // for main looper only
    void loop() {
        INFO("start main dispatcher");
        CHECK_TRUE(mMain, "loop() is available for main looper only");
        CHECK_TRUE(pthread_main(), "loop() can only be called in main thread");
        onSchedule();
    }
    
    void terminate() {
        INFO("terminate main dispatcher");
        CHECK_TRUE(mMain, "loop() is available for main looper only");
        AutoLock _l(mLock);
        mExitPending = true;
        mWait.signal();
        // NO WAIT HERE.
    }
};

static String MakeQueueName(const sp<ThreadScheduler>& scheduler) {
    return String::format("%s@%" PRIu32,
                          scheduler->mName.c_str(),
                          scheduler->mQueueID++);
}

// QueueScheduler is not shared with others.
// ALWAYS EXIT WITH JOBS IN QUEUE IF ALL REFERENCE ARE GONE:
struct QueueScheduler : public JobDelegate {
    sp<ThreadScheduler> mScheduler;
    sp<Job>             mDispatcher;
    
    enum { INACTIVE, ACTIVE };
    Atomic<UInt32>      mState;

    QueueScheduler(const sp<ThreadScheduler>& sched) :
    JobDelegate(MakeQueueName(sched), FOURCC('!que')),
    mScheduler(sched), mDispatcher(new Dispatcher(this)), mState(INACTIVE) {
        CHECK_FALSE(mScheduler.isNil());
    }
    
    ~QueueScheduler() {
        DEBUG("%s: release QueueScheduler", mName.c_str());
    }
    
    struct Dispatcher : public Job {
        wp<QueueScheduler> mDelegate;
        Dispatcher(const wp<QueueScheduler>& delegate) :
        Job(), mDelegate(delegate) { }
        
        virtual void onJob() {
            sp<QueueScheduler> delegate = mDelegate.retain();
            if (delegate.isNil()) return;
            delegate->onDispatch();
        }
    };
    
    // queue self into underlying scheduler.
    virtual void notify() {
        DEBUG("%s: queue a dispatcher", mName.c_str());
        UInt32 state = ACTIVE;
        if (mState.cas(state, INACTIVE)) {
            mScheduler->remove(this);
        }
        mScheduler->queue(new Dispatcher(this));
        mState.store(ACTIVE);
    }

    // NO wait() or sleep or loop in onJob,
    // or it will block underlying job scheduler
    virtual void onDispatch() {
        if (mState.load() == INACTIVE) return;
        Int64 next;
        dispatch(next);
        
        // no more jobs
        if (next < 0) {
            // exit on jobs finished
            DEBUG("%s: no more jobs", mName.c_str());
            mState.store(INACTIVE);
            return;
        }
        
        mScheduler->queue(new Dispatcher(this), next);
    }
};

//////////////////////////////////////////////////////////////////////////////////
struct JobHelper : public SharedObject {
    sp<JobDelegate>     mDelegate;

    JobHelper() : SharedObject(FOURCC('@job')), mDelegate(Nil) { }
};

Job::Job() : SharedObject(FOURCC('?job')) {
    
}

Job::Job(const sp<Looper>& loop) : SharedObject(FOURCC('?job')) {
    sp<JobHelper> jh = new JobHelper;
    jh->mDelegate = loop->mPartner;
    mPartner = jh->RetainObject();
}

Job::Job(const sp<DispatchQueue>& queue) : SharedObject(FOURCC('?job')) {
    sp<JobHelper> jh = new JobHelper;
    jh->mDelegate = queue->mPartner;
    mPartner = jh->RetainObject();
}

void Job::onFirstRetain() {
    
}

void Job::onLastRetain() {
    if (mPartner) {
        mPartner->ReleaseObject();
        mPartner = Nil;
    }
}

void Job::dispatch(Time after) {
    if (mPartner == Nil) {
        DEBUG("run job directly");
        execution();
        return;
    }

    sp<JobHelper> jh = mPartner;
    if (jh->mDelegate.isNil()) {
        DEBUG("run job directly");
        execution();
    } else {
        DEBUG("queue job");
        jh->mDelegate->queue(this, after);
    }
}

Bool Job::sync(Time deadline) {
    if (mPartner == Nil) {
        DEBUG("run job directly");
        execution();
        return True;
    }

    sp<JobHelper> jh = mPartner;
    if (jh->mDelegate.isNil()) {
        DEBUG("run job directly");
        execution();
        return True;
    } else {
        DEBUG("queue job");
        return jh->mDelegate->sync(this, deadline);
    }
}

void Job::cancel() {
    if (mPartner == Nil) return;
    
    sp<JobHelper> jh = mPartner;
    if (!jh->mDelegate.isNil()) {
        jh->mDelegate->remove(this);
    }
}

void Job::execution() {
    onJob();
}

static __thread Looper * lpCurrent = Nil;
struct Initilizer : public Job {
    Looper  *   mLooper;
    Initilizer() : Job() { }
    virtual void onJob() {
        lpCurrent = mLooper;
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
    }
    return lpMain;
}

sp<Looper> Looper::Current() {
    // no lock need, lpCurrent is a thread context
    return lpCurrent ? lpCurrent : Main();
}

// main looper
Looper::Looper() : SharedObject(FOURCC('?lop')) {
    sp<JobDelegate> delegate = new ThreadScheduler;
    mPartner = delegate->RetainObject();
    
    sp<Initilizer> job = new Initilizer;
    job->mLooper = this;
    delegate->queue(job);
}

Looper::Looper(const String& name, const eThreadType& type) : SharedObject(FOURCC('?lop')) {
    sp<ThreadScheduler> sched = new ThreadScheduler(name, type);
    mPartner = sched->RetainObject();
    
    sp<Initilizer> job = new Initilizer;
    job->mLooper = this;
    sched->queue(job);
}

void Looper::onFirstRetain() {
    DEBUG("onRetainObject");
}

void Looper::onLastRetain() {
    DEBUG("onLastRetain");
    if (this == lpMain) {
        INFO("releasing main looper");
        lpMain = Nil;
    }
    mPartner->ReleaseObject();
    mPartner = Nil;
}

void Looper::loop() {
    static_cast<ThreadScheduler *>(mPartner)->loop();
}

void Looper::terminate() {
    static_cast<ThreadScheduler *>(mPartner)->terminate();
}

void Looper::dispatch(const sp<Job>& job, Time delayUs) {
    static_cast<JobDelegate *>(mPartner)->queue(job, delayUs);
}

Bool Looper::sync(const sp<Job>& job, Time deadline) {
    return static_cast<JobDelegate *>(mPartner)->sync(job, deadline);
}

Bool Looper::remove(const sp<Job>& job) {
    return static_cast<JobDelegate *>(mPartner)->remove(job);
}

Bool Looper::exists(const sp<Job>& job) const {
    return static_cast<JobDelegate *>(mPartner)->exists(job);
}

void Looper::flush() {
    static_cast<JobDelegate *>(mPartner)->flush();
}

//////////////////////////////////////////////////////////////////////////////////
DispatchQueue::DispatchQueue(const sp<Looper>& looper) :
SharedObject(FOURCC('?que'), new QueueScheduler(looper->mPartner)) {
    mPartner->RetainObject();
}

void DispatchQueue::onFirstRetain() {
}

void DispatchQueue::onLastRetain() {
    DEBUG("DispatchQueue::onReleaseObject");
    mPartner->ReleaseObject();
    mPartner = Nil;
}

Bool DispatchQueue::sync(const sp<Job>& job, Time deadline) {
    sp<JobDelegate> delegate = mPartner;
    return delegate->sync(job, deadline);
}

void DispatchQueue::dispatch(const sp<Job>& job, Time us) {
    sp<JobDelegate> delegate = mPartner;
    delegate->queue(job, us);
}

Bool DispatchQueue::exists(const sp<Job>& job) const {
    sp<JobDelegate> delegate = mPartner;
    return delegate->exists(job);
}

Bool DispatchQueue::remove(const sp<Job>& job) {
    sp<JobDelegate> delegate = mPartner;
    return delegate->remove(job);
}

void DispatchQueue::flush() {
    sp<JobDelegate> delegate = mPartner;
    delegate->flush();
}

__END_NAMESPACE_ABE
