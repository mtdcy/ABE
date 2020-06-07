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


// File:    Looper.h
// Author:  mtdcy.chen
// Changes: 
//          1. 20160701     initial version
//

#ifndef ABE_HEADERS_LOOPER_H
#define ABE_HEADERS_LOOPER_H 

#include <ABE/core/Types.h>
#include <ABE/core/String.h>

/**
 * thread type
 * combine of nice, priority and sched policy
 */
enum eThreadType {
    kThreadLowest           = 0,
    kThreadBackgroud        = 16,
    kThreadNormal           = 32,
    kThreadForegroud        = 48,
    // for those who cares about the sched latency.
    // root permission maybe required
    kThreadSystem           = 64,
    kThreadKernel           = 80,
    // for those sched latency is critical.
    // real time sched (SCHED_RR) will be applied if possible.
    // root permission maybe required
    kThreadRealtime         = 96,
    // POSIX.1 requires an implementation to support only a minimum 32
    // distinct priority levels for the real-time policies.
    kThreadHighest          = 128,
    kThreadDefault          = kThreadNormal,
};

__BEGIN_NAMESPACE_ABE

/**
 * Java style thread, easy use of thread, no need to worry about thread control
 * Thread(new MyJob()).run();
 * @note we prefer looper instead of thread, so keep thread simple
 */
class Job;
class ABE_EXPORT Thread : public SharedObject {
    public:
        /**
         * create a suspend thread with runnable
         * @param runnable  reference to runnable object
         * @note thread is joinable until join() or detach()
         */
        Thread(const Object<Job>& runnable, const eThreadType type = kThreadDefault);
    
        /**
         * get current thread reference
         * if current thread is main thread return its ref
         */
        static Thread& Current();
        static Thread& Main();

        /**
         * thread state
         */
        enum eThreadState {
            kThreadInitializing,    ///< before run()
            kThreadRunning,         ///< after run() and before join()&detach()
            kThreadTerminated,      ///< after join() or detach()
        };
        eThreadState state() const;

        /**
         * set thread name before run
         * @note only available before run(), else undefined
         */
        Thread& setName(const String& name);

        /**
         * set thread type before run
         * @note only available before run(), else undefined
         */
        Thread& setType(const eThreadType type);

        /**
         * get thread name
         */
        const String& name() const;

        /**
         * get thread priority
         */
        eThreadType type() const;

        /**
         * start thread execution
         * put thread state from initial into running
         */
        Thread& run();

        /**
         * wait for this thread to terminate
         * @note only join once for a thread.
         * @note join() or detach() is neccessary even without run()
         */
        void join();

    public:
        /**
         * get the native thread handle
         * to enable extra control through native apis
         * @return return native thread handle
         */
        pthread_t native_thread_handle() const;

    private:
        struct NativeContext;
        Object<NativeContext>   mNative;
        Object<Job>             mJob;

    private:
        Thread();
        DISALLOW_EVILS(Thread);
};

// two methods to use Job
// 1. post a Job to Looper directly
// 2. attach a Looper to Job and run
class Looper;
class DispatchQueue;
class ABE_EXPORT Job : public SharedObject {
    public:
        Job();
        Job(const Object<Looper>&);
        Job(const Object<DispatchQueue>&);
        virtual ~Job();

        // make job execution
        // if no Looper bind, delay will be ignored
        // @param us    time to delay
        // @return return current ticks
        virtual size_t run(int64_t us = 0);

        // cancel execution
        virtual size_t cancel();

        // abstract interface
        virtual void onJob() = 0;

    public:
        // run Job directly, for job dispatcher
        void execution();   // -> onJob()
    
    protected:
        Object<Looper> mLooper;
        Object<DispatchQueue> mQueue;
        // current ticks, inc after execution complete
        Atomic<size_t> mTicks;
        DISALLOW_EVILS(Job);
};

struct JobDispatcher;
class ABE_EXPORT Looper : public SharedObject {
    public:
        /**
         * get 'main looper', prepare one if not exists.
         * @return return reference to 'main looper'
         */
        static Object<Looper>   Main();
    
        /**
         * get current looper
         * @return return reference to current looper
         */
        static Object<Looper>   Current();

        /**
         * create a looper
         */
        Looper();
        Looper(const String& name, const eThreadType& type = kThreadNormal);

    public:
        /**
         * get backend thread
         * @note no backend thread for main looper, return Thread::Null for main looper
         */
        Thread&     thread();

    public:
        /**
         * post a Job object to this looper.
         * runnable will be released when all refs gone.
         * @param what      - runnable object
         * @param delayUs   - delay time in us
         */
        void        post(const Object<Job>& what, int64_t delayUs = 0);

        /**
         * remove a Job object from this looper
         * @param what      - runnable object
         */
        void        remove(const Object<Job>& what);

        /**
         * test if a Job object is already in this looper
         * @param what      - runnable object
         */
        bool        exists(const Object<Job>& what) const;

        /**
         * flush Job objects from this looper
         */
        void        flush();

    public:
        /**
         * for main looper only 
         * can only be used in main thread and it will always block
         */
        void        loop();

        /**
         * for main looper only
         * can only be used in non-main threads
         */
        void        terminate();

    public:
        /**
         * profile looper, for debugging purpose
         */
        void        profile(int64_t interval = 5 * 1000000LL);

    private:
        virtual void onFirstRetain();
        virtual void onLastRetain();

        friend struct JobDispatcher;
        friend class DispatchQueue;
        Object<Looper>          mLooper;
        Object<JobDispatcher>   mJobDisp;

        DISALLOW_EVILS(Looper);
};

// for multi session share the same looper
class ABE_EXPORT DispatchQueue : public SharedObject {
    public:
        DispatchQueue(const Object<Looper>&);
        
        virtual ~DispatchQueue();
    
        const Object<Looper>& looper() const { return mLooper; }
        
    public:
        void    sync(const Object<Job>&);
    
        void    dispatch(const Object<Job>&, int64_t us = 0);
    
        bool    exists(const Object<Job>&) const;
        
        void    remove(const Object<Job>&);
        
        void    flush();
        
    private:
        virtual void onFirstRetain();
        virtual void onLastRetain();
    
        friend struct JobDispatcher;
        Object<Looper>          mLooper;
        Object<JobDispatcher>   mDispatcher;
        
        DISALLOW_EVILS(DispatchQueue);
};

__END_NAMESPACE_ABE
#endif // ABE_HEADERS_LOOPER_H
