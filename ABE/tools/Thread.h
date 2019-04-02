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


// File:    Thread.h
// Author:  mtdcy.chen
// Changes: 
//          1. 20160701     initial version
//

#ifndef _TOOLKIT_HEADERS_THREAD_H
#define _TOOLKIT_HEADERS_THREAD_H

#include <ABE/basic/Types.h>

__BEGIN_DECLS

/**
 * thread type
 * combine of nice, priority and sched policy
 */
enum eThreadType {
    kThreadLowest           = -32,
    kThreadBackgroud        = -16,
    kThreadNormal           = 0,
    kThreadForegroud        = 16,
    // for those who cares about the sched latency.
    kThreadSystem           = 32,
    kThreadKernel           = 48,
    // for those sched latency is critical.
    // real time sched (SCHED_RR) will be applied if possible.
    // root permission maybe required
    kThreadRealtime         = 64,
    // POSIX.1 requires an implementation to support only a minimum 32
    // distinct priority levels for the real-time policies.
    // we double this
    kThreadHighest          = 128,
    kThreadDefault          = kThreadNormal,
};

__END_DECLS

#ifdef __cplusplus 
#include <ABE/basic/String.h>
#include <ABE/object/Runnable.h>

__BEGIN_NAMESPACE_ABE
/**
 * Java style thread, easy use of thread, no need to worry about thread control
 * Thread(new MyRunnable()).run();
 * @note we prefer looper instead of thread, so keep thread simple
 */
class __ABE_EXPORT Thread {
    public:
        /**
         * yield current thread
         */
        static void         Yield();

        /**
         *
         */
        static void         Once(const sp<Runnable>&);

    public:
        /**
         * create a suspend thread with runnable
         * @param runnable  reference to runnable object
         * @note thread is joinable until join() or detach()
         */
        Thread(const sp<Runnable>& runnable, const eThreadType type = kThreadDefault);
        ~Thread();

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
        String& name() const;

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
         * is this thread joinable
         */
        bool joinable() const;

        /**
         * wait for this thread to terminate
         * @note only join once for a thread.
         * @note join() or detach() is neccessary even without run()
         */
        void join();

        /**
         * permits this thread to execute independently from the thread object
         * @note runnable must be independent from the thread object
         * @note join() or detach() is neccessary even without run()
         */
        void detach();

    public:
        /**
         * get the native thread handle
         * to enable extra control through native apis
         * @return return native thread handle
         */
        pthread_t native_thread_handle() const;
    
    public:
        static Thread Null;

    private:
        Thread() : mShared(NULL) { }
        sp<SharedObject> mShared;

    private:
        DISALLOW_EVILS(Thread);
};

__END_NAMESPACE_ABE
#endif // __cplusplus

#endif // _TOOLKIT_HEADERS_THREAD_H

