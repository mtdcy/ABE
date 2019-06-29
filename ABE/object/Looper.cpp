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
#include "basic/Log.h"

#include "stl/List.h"
#include "stl/Vector.h"
#include "stl/Queue.h"

#include "basic/Time.h"
#include "basic/Mutex.h"
#include "Looper.h"

// https://stackoverflow.com/questions/24854580/how-to-properly-suspend-threads
#include <signal.h>

#include "basic/compat/pthread.h"

__BEGIN_NAMESPACE_ABE

struct Job {
    Object<Runnable>    mRoutine;
    int64_t             mTime;

    Job() : mRoutine(NULL), mTime(0) { }
    Job(const Object<Runnable>& routine, int64_t delay) : mRoutine(routine),
    mTime(SystemTimeUs() + (delay < 0 ? 0 : delay)) { }

    bool operator<(const Job& rhs) const {
        return mTime < rhs.mTime;
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

    ABE_INLINE void start_profile(const Job& job) {
        ++num_job;
        last_exec = SystemTimeUs();
        if (job.mTime < last_exec) {
            ++num_job_late;
            job_late_time += (last_exec - job.mTime);
        } else {
            ++num_job_early;
            job_early_time += (job.mTime - last_exec);
        }
    }

    ABE_INLINE void end_profile(const Job& job) {
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

struct JobDispatcher : public Runnable {
    // internal context
    Stat                            mStat;  // only used inside looper

    // mutable context, access with lock
    mutable Mutex                   mJobLock;
    mutable LockFree::Queue<Job>    mJobs;
    mutable List<Job>               mTimedJobs;
    Vector<void *>                  mUserContext;

    JobDispatcher() : Runnable() { }

    void profile(int64_t interval = 5 * 1000000LL) {
        mStat.profile(interval);
    }

    void post(const Object<Runnable>& routine, int64_t delay = 0) {
        Job job(routine, delay);

        // using lockfree queue to speed up post()
        // but this will cause remove() and exists() more complex
        if (delay == 0) {
            mJobs.push(job);
            signal();
            return;
        }

        // else push routine into mTimedJobs
        AutoLock _l(mJobLock);

        // go through list should be better than sort()
        List<Job>::iterator it = mTimedJobs.begin();
        while (it != mTimedJobs.end() && (*it).mTime < job.mTime) {
            ++it;
        }

        if (it == mTimedJobs.begin()) {
            // have signal before insert(), as insert() will change begin()
            signal();
        }
        mTimedJobs.insert(it, job);
    }

    // return reference to runnable if job is ready, or return NULL
    // next: set to -1 if not job exists, or next job time in us
    bool pop(Job& job, int64_t * next) {
        AutoLock _l(mJobLock);

        *next = -1;     // job not exists

        if (mTimedJobs.size()) {
            Job& head = mTimedJobs.front();
            const int64_t now = SystemTimeUs();
            // with 1ms jitter:
            // our SleepForInterval and waitRelative based on ns,
            // but os backend implementation can not guarentee it
            // miniseconds precise is the least.
            if (head.mTime <= now + 1000LL) {
                job = head;
                mTimedJobs.pop();
                return true;
            }

            *next = head.mTime - now;
        }

        if (mJobs.pop(job)) {
            return true;
        } else {
            return false;
        }
    }

    void remove(const Object<Runnable>& routine) {
        AutoLock _l(mJobLock);

        // move mJobs -> mTimedJobs
        if (mJobs.size()) {
            Job tmp;
            while (mJobs.pop(tmp)) {
                mTimedJobs.push(tmp);
            }
            mTimedJobs.sort();
        }

        bool head = false;
        List<Job>::iterator it = mTimedJobs.begin();
        while (it != mTimedJobs.end()) {
            if ((*it).mRoutine == routine) {
                it = mTimedJobs.erase(it);
                if (it == mTimedJobs.begin()) head = true;
            } else {
                ++it;
            }
        }

        if (head) signal();
    }

    bool exists(const Object<Runnable>& routine) const {
        AutoLock _l(mJobLock);

        // move mJobs -> mTimedJobs
        if (mJobs.size()) {
            Job tmp;
            while (mJobs.pop(tmp)) {
                mTimedJobs.push(tmp);
            }
            mTimedJobs.sort();
        }

        List<Job>::const_iterator it = mTimedJobs.cbegin();
        for (; it != mTimedJobs.cend(); ++it) {
            if ((*it).mRoutine == routine) {
                return true;
            }
        }
        return false;
    }

    void flush() {
        AutoLock _l(mJobLock);
        mTimedJobs.clear();
        mJobs.clear();
        signal();
    }

    virtual void loop() = 0;
    virtual void signal() = 0;
    virtual void terminate(bool wait) = 0;
};

static __thread JobDispatcher * __tls = NULL;
struct NormalJobDispatcher : public JobDispatcher {
    Thread                  mThread;
    bool                    mTerminated;
    bool                    mRequestExit;
    bool                    mWaitForJobDidFinished;
    mutable Mutex           mLock;
    Condition               mWait;

    NormalJobDispatcher() : JobDispatcher(), mThread(this),
    mTerminated(false), mRequestExit(false), mWaitForJobDidFinished(false) { }

    virtual void signal() {
        AutoLock _l(mLock);
        mWait.signal();
    }

    virtual void terminate(bool wait) {
        {
            AutoLock _l(mLock);
            if (mTerminated) return;
            mRequestExit            = true;
            mWaitForJobDidFinished  = wait;
            mWait.signal();
        }
        
        if (wait)   mThread.join();
        else        mThread.detach();
    }

    virtual void run() {
        __tls = this;
        
        mStat.start();
        
        for (;;) {
            int64_t next = 0;
            Job job;
            if (pop(job, &next)) {
                mStat.start_profile(job);
                job.mRoutine->run();
                mStat.end_profile(job);
                continue;
            }

            AutoLock _l(mLock);
            if (next > 0) {
                if (mRequestExit && !mWaitForJobDidFinished) {
                    DEBUG("exiting...");
                    break;
                }
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
        mTerminated = false;
        mWait.broadcast();
        mStat.stop();
        __tls = NULL;
    }
    
    virtual void loop() {
        mThread.run();
    }
};

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

static volatile bool g_quit = false;
static pthread_t g_threadid;
#if defined(_WIN32) || defined(__MINGW32__)
// signal on win32 is very simple, we need some help
#define SIG_RESUME 	SIGINT
static ABE_INLINE void wait_for_signals() {
    DEBUG("goto sleep");
    SleepForInterval(1000000000LL);
    DEBUG("wake up from sleep");
}

static void signal_exit(int signo) {
    g_quit = true;
}

static void signal_null(int signo) {
}

static ABE_INLINE void init_signals() {
    DEBUG("install signal handlers");
    signal(SIG_RESUME, signal_null);
}
#else
#define SIG_RESUME      SIGUSR1
static ABE_INLINE void wait_for_signals() {
    /* save errno to keep it unchanged in the interrupted thread. */
    const int saved = errno;

    sigset_t sigset;
    sigfillset(&sigset);
#if 1
    sigdelset(&sigset, SIG_RESUME);
    sigdelset(&sigset, SIGINT);
#else
    sigemptyset(&sigset);
#endif
    CHECK_EQ(sigsuspend(&sigset), -1);  // return with errno set
    CHECK_TRUE(errno == EINTR);
    DEBUG("main: sigsuspend interrupted");

    /* restore errno. */
    errno = saved;
}

static ABE_INLINE void uninstall(int sig) {
    struct sigaction act;
    sigemptyset(&act.sa_mask);
    sigaddset(&act.sa_mask, sig);
    act.sa_flags = SA_RESETHAND;
    CHECK_EQ(sigaction(sig, &act, NULL), 0);
}

static void sigaction_exit(int signum, siginfo_t *info, void *vcontext) {
    INFO("sig %s @ [%d, %d]", signame(info->si_signo), info->si_pid, info->si_uid);
    g_quit = true;
#if 0 // cause sigsuspend assert, why ???
    uninstall(SIG_RESUME);
    uninstall(SIGINT);
#endif
    INFO("main: exit...");
}

static void sigaction_null(int sig, siginfo_t *info, void *vcontext) {
    DEBUG("sig %s @ [%d, %d]", signame(info->si_signo), info->si_pid, info->si_uid);
    // NOTHING
}

static ABE_INLINE void init_signals() {
    g_threadid = pthread_self();
    
    // install signal handlers
    struct sigaction act;
    sigemptyset(&act.sa_mask);
    sigaddset(&act.sa_mask, SIG_RESUME);
    sigaddset(&act.sa_mask, SIGINT);
    act.sa_flags = SA_RESTART | SA_SIGINFO;

    act.sa_sigaction = sigaction_null;
    CHECK_EQ(sigaction(SIG_RESUME, &act, NULL), 0);

    // XXX: make sure main looper can terminate by ctrl-c
    act.sa_sigaction = sigaction_exit;
    CHECK_EQ(sigaction(SIGINT, &act, NULL), 0);
}
#endif

// job dispatcher for main looper, also handle signals of the process
struct MainJobDispatcher : public JobDispatcher {
    volatile bool   looping;

    MainJobDispatcher() : JobDispatcher(), looping(false) {
        init_signals();
    }

    ~MainJobDispatcher() {
        // wait other loopers to terminate
    }

    virtual void signal() {
        DEBUG("main: signal");
        pthread_kill(g_threadid, SIG_RESUME);
    }

    virtual void terminate(bool wait) {
        g_quit = true;
        if (!pthread_equal(g_threadid, pthread_self())) {
            DEBUG("main: terminate by others");
            pthread_kill(g_threadid, SIG_RESUME);
        }
    }

    virtual void run() {
        mStat.start();
        while (!g_quit) {
            int64_t next = 0;
            Job job;
            if (pop(job, &next)) {
                mStat.start_profile(job);
                job.mRoutine->run();
                mStat.end_profile(job);
                continue;
            }

            if (next < 0) {
                DEBUG("main: wait for signals");
                mStat.sleep();
                wait_for_signals();
                mStat.wakeup();
                DEBUG("main: wakeup");
            } else if (next > 0) {
                DEBUG("main: wait for next job, %" PRId64, next);
                // sleep for next job, will be interrupted by signals
                mStat.sleep();
                if (SleepForInterval(next * 1000LL) == false) {
                    DEBUG("main: interrupt");
                }
                mStat.wakeup();
            }
        }
        mStat.stop();
    }
    
    virtual void loop() {
        run();
    }
};

//////////////////////////////////////////////////////////////////////////////////
Object<Looper> Looper::Current() {
    if (__tls == NULL) return NULL;

    Object<Looper> current = new Looper;
    current->mShared = __tls;
    return current;
}

// main looper without backend thread
// auto clear __main on last ref release
static Looper * __main = NULL;
Object<Looper> Looper::Main() {
    if (__main == NULL) {   // it is ok without lock
        DEBUG("init main looper");
        CHECK_TRUE(pthread_main(), "main looper must be intialized in main()");
        __main = new Looper;
        __main->mShared = new MainJobDispatcher;
        pthread_setname("main");
    }
    return __main;
}

// always keep a ref to global looper
// client have to clear it by set global looper to NULL
static Object<Looper> __global = NULL;
Object<Looper> Looper::Global() { return __global; }
void Looper::SetGlobal(const Object<Looper>& lp) { __global = lp; }

Object<Looper> Looper::Create(const String& name, const eThreadType& type) {
    Object<Looper> looper = new Looper;
    looper->thread().setName(name).setType(type);
    return looper;
}

Looper::Looper() : SharedObject(OBJECT_ID_LOOPER) { }

void Looper::onFirstRetain() {
    if (mShared == NULL) {
        mShared = new NormalJobDispatcher;
    }
}

void Looper::onLastRetain() {
    Object<JobDispatcher> shared = mShared;
    shared->flush();
    
    mShared = NULL;
    // clear main pointer
    if (this == __main) {
        INFO("on main looper last ref!!!");
        __main = NULL;
    }
}

Thread& Looper::thread() const {
    if (this == __main) {
        return Thread::Main();
    }
    
    Object<NormalJobDispatcher> shared = mShared;
    return shared->mThread;
}

void Looper::loop() {
    Object<JobDispatcher> shared = mShared;
    shared->loop();
}

void Looper::terminate(bool wait) {
    Object<JobDispatcher> shared = mShared;
    shared->terminate(wait);
}

void Looper::profile(int64_t interval) {
    Object<JobDispatcher> shared = mShared;
    shared->profile(interval);
}

void Looper::post(const Object<Runnable>& routine, int64_t delayUs) {
    Object<JobDispatcher> shared = mShared;
    shared->post(routine, delayUs);
}

void Looper::remove(const Object<Runnable>& routine) {
    Object<JobDispatcher> shared = mShared;
    shared->remove(routine);
}

bool Looper::exists(const Object<Runnable>& routine) const {
    Object<JobDispatcher> shared = mShared;
    return shared->exists(routine);
}

void Looper::flush() {
    Object<JobDispatcher> shared = mShared;
    shared->flush();
}

String Looper::string() const {
    Object<JobDispatcher> shared = mShared;
    String info;
#if 0
    String info = String::format("looper %s: jobs %zu",
            looper->mDispatcher->mName.c_str(),
            looper->mDispatcher->mJobs.size());

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
    Object<JobDispatcher> shared = mShared;

    AutoLock _l(shared->mJobLock);
    shared->mUserContext.push(user);
    return shared->mUserContext.size() - 1;
}

void Looper::bind(size_t id, void *user) {
    Object<JobDispatcher> shared = mShared;

    AutoLock _l(shared->mJobLock);
    CHECK_LE(id, shared->mUserContext.size());
    if (id == shared->mUserContext.size()) {
        shared->mUserContext.push(user);
    } else {
        shared->mUserContext[id] = user;
    }
}

void * Looper::user(size_t id) const {
    Object<JobDispatcher> shared = mShared;

    AutoLock _l(shared->mJobLock);
    if (id >= shared->mUserContext.size())
        return NULL;
    return shared->mUserContext[id];
}

__END_NAMESPACE_ABE
