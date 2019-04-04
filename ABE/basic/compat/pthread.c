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


// File:    mutex.h
// Author:  mtdcy.chen
// Changes: 
//          1. 20161012     initial version
//
#if defined(__linux__)
#define _GNU_SOURCE
#elif defined(__APPLE__)
#define _DARWIN_C_SOURCE
#endif

#include "Config.h"
#include "basic/compat/pthread.h"

#include <pthread.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_SYS_PRCTL_H
#include <sys/prctl.h>
#endif

#ifdef __ANDROID__
#include <sys/resource.h>
#elif defined(_WIN32) || defined(__MINGW32__)
#include <processthreadsapi.h>
#else
#include <sys/syscall.h>
#endif

#include <errno.h>

int pthread_setname_mpx(const char* name) {
    int ret = 0;
#if defined(HAVE_PTHREAD_SETNAME_NP)
#ifdef __APPLE__ 
    pthread_setname_np(name);
#else
    pthread_setname_np(pthread_self(), name);
#endif
#elif defined(HAVE_SYS_PRCTL_H)
    prctl(PR_SET_NAME, name.c_str(), 0, 0, 0);
#else
#error "set thread name is not implemented."
#endif
    return ret;
}

int pthread_getname_mpx(pthread_t thread, char *name, size_t n) {
#ifdef __ANDROID__ 
    char tmp[32];
    int ret = prctl(PR_GET_NAME, tmp, 0, 0, 0);
    if (ret != 0) {
        strncpy(name, "unknown", n);
    } else {
        strncpy(name, tmp, n);
    }
    return ret;
#else
    int ret = 0;
    ret = pthread_getname_np(thread, name, n);
    return ret;
#endif
}

pid_t mpx_gettid() {
#ifdef __ANDROID__ 
    return pthread_gettid_np(pthread_self());
    //return gettid();
#elif defined(__APPLE__)
    uint64_t id;
    pthread_threadid_np(pthread_self(), &id);
    return id;
    //return syscall(SYS_thread_selfid);
#elif defined(HAVE_GETTID)
    return gettid();
#elif defined(HAVE_PTHREAD_GETTHREADID_NP)
    return pthread_getthreadid_np();
#elif defined(__MINGW32__)
    // trick
    return getpid() + pthread_self() - 1;
#else // glibc
    return syscall(__NR_gettid);
#endif
}

void pthread_yield_mpx() {
#ifdef __ANDROID__
    sched_yield();
#elif defined(__MINGW32__)
    sched_yield();
#elif defined(__APPLE__)
    pthread_yield_np();
#else
    pthread_yield();
#endif
}

int pthread_main_mpx() {
#ifdef __APPLE__
    return pthread_main_np();
#else
    return getpid() == mpx_gettid();
#endif
}

