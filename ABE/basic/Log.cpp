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


// File:    Log.cpp
// Author:  mtdcy.chen
// Changes: 
//          1. 20160701     initial version
//
#if defined(__APPLE__)
#include "basic/compat/pthread_macos.h"
#include "basic/compat/time_macos.h"
#elif defined(_WIN32) || defined(__MINGW32__)
#include "basic/compat/pthread_win32.h"
#include "basic/compat/time_win32.h"
#else
#include "basic/compat/pthread_linux.h"
#include "basic/compat/time_linux.h"
#endif

#include <stdio.h> // printf
#ifdef __ANDROID__
#include <android/log.h>
#endif

#include "Log.h"
#include "debug/backtrace.h"

#if defined(_WIN32) || defined(__MINGW32__)
#define PRIpid_t    "lld"
#else
#define PRIpid_t    "d"
#endif

__BEGIN_DECLS

// NOTE:
//  1. should avoid having dependency to others as possible as you can
//  2. must avoid calling others which uses Log.

#if 0
static void _default_callback(const char *tag, int level, const char *text) {
#ifdef __ANDROID__
    // android does have an log driver, so no need service here.
    static int android_levels[] = {
        ANDROID_LOG_FATAL,
        ANDROID_LOG_ERROR,
        ANDROID_LOG_WARN,
        ANDROID_LOG_INFO,
        ANDROID_LOG_DEBUG
    };

    __android_log_write(android_levels[level], tag, text);
#else
    static const char * LEVELS[] = {
        "F",
        "E",
        "W",
        "I",
        "D",
        NULL
    };

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    char name[32];
    pthread_getname_mpx(pthread_self(), name, 32);
    if (name[0] == '\0') {
        fprintf(stdout, "[%08.03f] [%06d] [%-14.14s] [%1s] >> %s\n",
                nseconds(ts) / 1E9,
                pthread_gettid(),
                tag,
                LEVELS[level],
                text);
    } else {
        fprintf(stdout, "[%08.03f] [%-6.6s] [%-14.14s] [%1s] >> %s\n",
                nseconds(ts) / 1E9,
                name,
                tag,
                LEVELS[level],
                text);
    }
#endif
}
#endif

static bool __init = false;
static __ABE_INLINE void _init() {
#if defined(_WIN32) || defined(__MINGW32__)
    setvbuf(stdout, 0, _IOLBF, 2048);
#endif
    __init = true;
}

typedef void (*Callback_t)(const char *);
static Callback_t   __callback = NULL;

void LogSetCallback(Callback_t cb) { __callback = cb; }

void LogPrint(const char *      tag,
        enum eLogLevel    level,
        const char *      func,
        size_t            line,
        const char *      format,
        ...) {

    if (__builtin_expect(__init == false, false)) _init();

    va_list ap;
    char buf[1024];

    va_start(ap, format);
    vsnprintf(buf, 1024, format, ap);
    va_end(ap);
    
    if (__callback) {
        __callback(buf);
        return;
    }

    static const char * LEVELS[] = {
        "F",
        "E",
        "W",
        "I",
        "D",
        NULL
    };

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts); // time since Epoch.
    char name[32];
    pthread_getname(pthread_self(), name, 32);
    if (name[0] == '\0') {
        fprintf(stdout, "[%08.03f][%07" PRIpid_t "][%-7.7s][%1s][%14.14s:%zu] : %s\n",
                nseconds(ts) / 1E9,
                pthread_gettid(),
                tag,
                LEVELS[level],
                func,
                line,
                buf);
    } else {
        fprintf(stdout, "[%08.03f][%-7.7s][%-7.7s][%1s][%14.14s:%zu] : %s\n",
                nseconds(ts) / 1E9,
                name,
                tag,
                LEVELS[level],
                func,
                line,
                buf);
    }
#if defined(_WIN32) || defined(__MINGW32__)
    fflush(stdout);
#endif

    if (level == LOG_FATAL) {
        BACKTRACE();
        __builtin_trap();
    }
}

__END_DECLS
