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

#ifndef _TOOLKIT_HEADERS_LOOPER_H
#define _TOOLKIT_HEADERS_LOOPER_H 

#ifdef __cplusplus
#include <ABE/object/Runnable.h>
#include <ABE/tools/Thread.h>
__BEGIN_NAMESPACE_ABE
class Looper : public SharedObject {
    public:
        /**
         * get 'main looper', prepare one if not exists.
         * @return return reference to 'main looper'
         */
        static sp<Looper>   Main();

        /**
         * get current looper
         * @return return reference to current looper
         */
        static sp<Looper>   Current();

        /**
         * create a looper
         */
        static sp<Looper>   Create(const String& name, const eThreadType& type = kThreadNormal)
        { return new Looper(name, type); }

    protected:
        /**
         * create a looper with given name
         * @note using Thread::Current() to enable extra control of backend thread
         */
        Looper(const String& name, const eThreadType& type);

        /**
         * delete looper object
         * if runnable is independent from looper object, it is ok to
         * delete without terminate()
         */
        virtual ~Looper();

    public:
        /**
         * get backend thread
         * @note no backend thread for main looper
         */
        Thread& thread() const;

        /**
         * get looper name
         */
        String& name() const;

        /**
         * run current looper
         * @note it's ok to post runnable inside before loop
         */
        void loop();

        /**
         * terminate the backend thread
         * @param waitUntilJobsDidFinished  terminate with or without jobs
         * @note post to looper when terminating or after terminated will fail
         * @note terminate is neccessary even without loop()
         */
        void terminate(bool waitUntilJobsDidFinished = false);

        /**
         * get infomation about this looper
         */
        String      string() const;

        /**
         * bind a user context pointer to looper
         * so it is easy to retrieve from Runnable/Event/Handler
         * @param id        id of user context
         * @param opaque    user context
         * @return return context id
         * @note bind NULL to unbind
         */
        size_t      bind(void *user);
        void        bind(size_t id, void *user);
        void *      user(size_t id) const;

    public:
        /**
         * post a Runnable object to this looper.
         * runnable will be released when all refs gone.
         * @param what      - runnable object
         * @param delayUs   - delay time in us
         */
        void post(const sp<Runnable>& what, int64_t delayUs = 0);

        /**
         * remove a Runnable object from this looper
         * @param what      - runnable object
         */
        void remove(const sp<Runnable>& what);

        /**
         * test if a Runnable object is already in this looper
         * @param what      - runnable object
         */
        bool exists(const sp<Runnable>& what) const;

        /**
         * flush Runnable objects from this looper
         */
        void flush();

    private:
        Looper() : mShared(NULL) { }
        sp<SharedObject>    mShared;

    private:
        DISALLOW_EVILS(Looper);
};
__END_NAMESPACE_ABE
#endif // __cplusplus

#ifdef __cplusplus
using __NAMESPACE_ABE::Looper;
using __NAMESPACE_ABE::Runnable;
#else
typedef struct Looper   Looper;
typedef struct Runnable Runnable;
#endif

__BEGIN_DECLS

Looper *    SharedLooperCreate(const char * name);
#define SharedLooperRetain(r)   (Looper *)SharedObjectRetain((SharedObject *)r)
#define SharedLooperRelease(r)  SharedObjectRelease((SharedObject *)r)

void        SharedLooperLoop(Looper *);
void        SharedLooperTerminate(Looper *);
void        SharedLooperTerminateAndWait(Looper *);

void        SharedLooperPostRunnable(Looper *, Runnable *);
void        SharedLooperPostRunnableWithDelay(Looper *, Runnable *, int64_t);
void        SharedLooperRemoveRunnable(Looper *, Runnable *);
bool        SharedLooperFindRunnable(Looper *, Runnable *);
void        SharedLooperFlush(Looper *);

__END_DECLS

#endif // _TOOLKIT_HEADERS_LOOPER_H
