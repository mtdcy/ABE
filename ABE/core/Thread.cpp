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


// File:    Thread.cpp
// Author:  mtdcy.chen
// Changes:
//          1. 20160701     initial version
//

#define LOG_TAG   "Thread"
#define LOG_NDEBUG 0
#include "Log.h"

#include "stl/List.h"
#include "stl/Vector.h"

#include "System.h"
#include "Mutex.h"
#include "Looper.h"

#include <sched.h>

#include "compat/pthread.h"

#include <errno.h>

#define JOINABLE 1

USING_NAMESPACE_ABE

__BEGIN_DECLS

static const Char * NAMES[] = {
    "Lowest",
    "Backgroud",
    "Normal",
    "Foreground",
    "System",
    "Kernel",
    "Realtime",
    "Realtime",
    "Highest"
};

// interpret thread type to nice & priority
// kThreadLowest - kThreadForegroud :   SCHED_OTHER
// kThreadSystem - kThreadKernel:       SCHED_FIFO
// kThreadRealtime - kThreadHighest:    SCHED_RR
static void SetThreadType(eThreadType type) {
    sched_param old;
    sched_param par;
    int policy;
    pthread_getschedparam(pthread_self(), &policy, &old);
    DEBUG("policy %d, sched_priority %d", policy, old.sched_priority);
    if (type < kThreadSystem) {
        policy = SCHED_OTHER;
        int min = sched_get_priority_min(SCHED_OTHER);
        int max = sched_get_priority_max(SCHED_OTHER);
        // make sure kThreadDefault map to default priority
        par.sched_priority = old.sched_priority;
        if (type >= kThreadDefault)
            par.sched_priority += ((max - old.sched_priority) * (type - kThreadDefault)) / (kThreadSystem - kThreadDefault);
        else
            par.sched_priority += ((min - old.sched_priority) * (type - kThreadDefault)) / (kThreadLowest - kThreadDefault);
    } else if (type < kThreadRealtime) {
        policy = SCHED_FIFO;
        int min = sched_get_priority_min(SCHED_FIFO);
        int max = sched_get_priority_max(SCHED_FIFO);
        par.sched_priority = min + ((max - min) * (type - kThreadSystem)) / (kThreadRealtime - kThreadSystem);
    } else {
        policy = SCHED_RR;
        int min = sched_get_priority_min(SCHED_RR);
        int max = sched_get_priority_max(SCHED_RR);
        par.sched_priority = min + ((max - min) * (type - kThreadRealtime)) / (kThreadHighest - kThreadRealtime);
    }
    int err = pthread_setschedparam(pthread_self(), policy, &par);
    switch (err) {
        case 0:
            DEBUG("%s => policy %d, sched_priority %d",
                    NAMES[type/16], policy, par.sched_priority);
            break;
        case EINVAL:
            ERROR("%s => %d %d failed, Invalid value for policy.",
                    NAMES[type/16], policy, par.sched_priority);
            break;
        case ENOTSUP:
            ERROR("%s => %d %d failed, Invalid value for scheduling parameters.",
                    NAMES[type/16], policy, par.sched_priority);
            break;
        case ESRCH:
            ERROR("%s => %d %d failed, Non-existent thread thread.",
                    NAMES[type/16], policy, par.sched_priority);
            break;
        default:
            ERROR("%s => %d %d failed, pthread_setschedparam failed, err %d|%s",
                    NAMES[type/16], policy, par.sched_priority, err, strerror(err));
            break;
    }
}

struct ThreadContext : public SharedObject {
    void *      mUser;
    void        (*mUserEntry)(void *);
    eThreadType mType;
};

static void * ThreadEntry(void * p) {
    ThreadContext * context = static_cast<ThreadContext *>(p);
    DEBUG("enter ThreadEntry");
    
    SetThreadType(context->mType);
    
    context->mUserEntry(context->mUser);
    
    DEBUG("exit ThreadEntry");
    context->ReleaseObject();
    pthread_exit(Nil);
    return Nil;    // just fix build warnings
}

eThreadStatus CreateThread(eThreadType type, void (*userEntry)(void *), void * user) {
    DEBUG("create thread of %s", NAMES[type/16]);
    // create new thread
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    // use joinable thread.
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    ThreadContext * context = new ThreadContext;
    context->mUser          = user;
    context->mUserEntry     = userEntry;
    context->mType          = type;
    
    pthread_t thread;
    switch (pthread_create(&thread, &attr, ThreadEntry, context->RetainObject())) {
        case EAGAIN:
            FATAL("Insufficient resources to create another thread.");
            break;
        case EINVAL:
            FATAL("Invalid settings in attr.");
            break;
        default:
            FATAL("pthread_create failed");
            break;
        case 0:
            break;
    }

    return kThreadOK;
}
__END_DECLS
