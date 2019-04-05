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

#if defined(__APPLE__)
#include "basic/compat/pthread_macos.h"
#else
#endif

#define LOG_TAG   "Looper"
//#define LOG_NDEBUG 0
#include "basic/Log.h"

#include "stl/List.h"
#include "stl/Vector.h"
#include "stl/Queue.h"

#include "basic/Time.h"
#include "tools/Mutex.h"
#include "Looper.h"

// https://stackoverflow.com/questions/24854580/how-to-properly-suspend-threads
#include <signal.h>

__BEGIN_NAMESPACE_ABE

struct __ABE_HIDDEN Job {
    sp<Runnable>    mRoutine;
    int64_t         mTime;

    Job() : mRoutine(NULL), mTime(0) { }
    Job(const sp<Runnable>& routine, int64_t delay) : mRoutine(routine),
    mTime(SystemTimeUs() + (delay < 0 ? 0 : delay)) { }

    bool operator<(const Job& rhs) const {
        return mTime < rhs.mTime;
    }
};

struct __ABE_HIDDEN Stat {
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

    __ABE_INLINE Stat() {
        start_time = sleep_time = exec_time = 0;
        num_job = num_job_late = num_job_early = 0;
        job_late_time = job_early_time = 0;
        profile_enabled = false;
    }

    __ABE_INLINE void start() {
        start_time = SystemTimeUs();
    }

    __ABE_INLINE void stop() {
    }

    __ABE_INLINE void profile(int64_t interval) {
        profile_enabled = true;
        profile_interval = interval;
        last_profile_time = SystemTimeUs();
    }

    __ABE_INLINE void start_profile(const Job& job) {
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

    __ABE_INLINE void end_profile(const Job& job) {
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

    __ABE_INLINE void sleep() {
        last_sleep = SystemTimeUs();
    }

    __ABE_INLINE void wakeup() {
        const int64_t ns = SystemTimeUs() - last_sleep;
        sleep_time += ns;
    }
};

static __thread Looper * __tls = NULL;
struct __ABE_HIDDEN JobDispatcher : public Runnable {
    Looper * _interface;    // hack for Looper::Current()
    
    // internal context
    String                          mName;
    Stat                            mStat;  // only used inside looper

    // mutable context, access with lock
    mutable Mutex                   mJobLock;
    mutable LockFree::Queue<Job>    mJobs;
    mutable List<Job>               mTimedJobs;
    Vector<void *>                  mUserContext;

    JobDispatcher(const String& name) : Runnable(), mName(name) { }

    void profile(int64_t interval = 5 * 1000000LL) {
        mStat.profile(interval);
    }

    void post(const sp<Runnable>& routine, int64_t delay = 0) {
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

    void remove(const sp<Runnable>& routine) {
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

    bool exists(const sp<Runnable>& routine) const {
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

    void clear() {
        AutoLock _l(mJobLock);
        mTimedJobs.clear();
        mJobs.clear();
        signal();
    }

    virtual void signal() = 0;
    virtual void terminate(bool wait) = 0;
};

struct __ABE_HIDDEN NormalJobDispatcher : public JobDispatcher {
    bool                    mTerminated;
    bool                    mRequestExit;
    bool                    mWaitForJobDidFinished;
    mutable Mutex           mLock;
    Condition               mWait;

    NormalJobDispatcher(const String& name) : JobDispatcher(name),
    mTerminated(false), mRequestExit(false), mWaitForJobDidFinished(false) { }

    virtual void signal() {
        AutoLock _l(mLock);
        mWait.signal();
    }

    virtual void terminate(bool wait) {
        AutoLock _l(mLock);
        if (mTerminated) return;
        mRequestExit            = true;
        mWaitForJobDidFinished  = wait;
        mWait.signal();
    }

    virtual void run() {
        __tls = _interface;
        
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
};

static __ABE_INLINE const char * signame(int signo) {
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
static __ABE_INLINE void wait_for_signals() {
    DEBUG("goto sleep");
    SleepForInterval(1000000000LL);
    DEBUG("wake up from sleep");
}

static void signal_exit(int signo) {
    g_quit = true;
}

static void signal_null(int signo) {
}

static __ABE_INLINE void init_signals() {
    DEBUG("install signal handlers");
    signal(SIG_RESUME, signal_null);
}
#else
#define SIG_RESUME      SIGUSR1
static __ABE_INLINE void wait_for_signals() {
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

static __ABE_INLINE void uninstall(int sig) {
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

static __ABE_INLINE void init_signals() {
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
struct __ABE_HIDDEN MainJobDispatcher : public JobDispatcher {
    volatile bool   looping;

    MainJobDispatcher() : JobDispatcher("main"), looping(false) {
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
};

struct __ABE_HIDDEN SharedLooper : public SharedObject {
    sp<JobDispatcher>   mDispatcher;
    Thread *            mThread;

    // for main looper
    SharedLooper() : SharedObject(), mDispatcher(new MainJobDispatcher), mThread(NULL) {
        pthread_setname("main");
    }

    // for normal looper
    SharedLooper(const String& name, eThreadType type) :
        SharedObject(), mDispatcher(new NormalJobDispatcher(name)), mThread(new Thread(mDispatcher)) {
            mThread->setName(name).setType(type);
        }

    virtual ~SharedLooper() { if (mThread) delete mThread; }
};

//////////////////////////////////////////////////////////////////////////////////
static Looper * main_looper = NULL;

sp<Looper> Looper::Current() {
    return __tls;
}

// main looper without backend thread
sp<Looper> Looper::Main() {
    if (main_looper == NULL) {
	DEBUG("init main looper");
        CHECK_TRUE(pthread_main(), "main looper must be intialized in main()");
        main_looper = new Looper;
        sp<SharedLooper> looper = new SharedLooper;
        main_looper->mShared = looper;
    }
    return main_looper;
}

Looper::Looper(const String& name, const eThreadType& type) : SharedObject(),
    mShared(new SharedLooper(name, type))
{
    sp<SharedLooper> looper = mShared;
    looper->mDispatcher->_interface = this;     // hack for Looper::Current
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
    if (looper->mThread)    return *looper->mThread;
    else                    return Thread::Null;
}

void Looper::loop() {
    sp<SharedLooper> looper = mShared;
    if (looper->mThread) {
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

void Looper::profile(int64_t interval) {
    sp<SharedLooper> looper = mShared;
    looper->mDispatcher->profile(interval);
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

    AutoLock _l(looper->mDispatcher->mJobLock);
    looper->mDispatcher->mUserContext.push(user);
    return looper->mDispatcher->mUserContext.size() - 1;
}

void Looper::bind(size_t id, void *user) {
    sp<SharedLooper> looper = mShared;

    AutoLock _l(looper->mDispatcher->mJobLock);
    CHECK_LE(id, looper->mDispatcher->mUserContext.size());
    if (id == looper->mDispatcher->mUserContext.size()) {
        looper->mDispatcher->mUserContext.push(user);
    } else {
        looper->mDispatcher->mUserContext[id] = user;
    }
}

void * Looper::user(size_t id) const {
    sp<SharedLooper> looper = mShared;

    AutoLock _l(looper->mDispatcher->mJobLock);
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
