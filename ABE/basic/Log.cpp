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

#include <stdio.h> // printf
#ifdef __ANDROID__
#include <android/log.h>
#endif

#include "Log.h"
#include "compat/time.h"
#include <pthread.h>
#include "compat/pthread.h"
#include "debug/backtrace.h"

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
    absolute_time(&ts);
    char name[32];
    pthread_getname_mpx(pthread_self(), name, 32);
    if (name[0] == '\0') {
        fprintf(stdout, "[%08.03f] [%06d] [%-14.14s] [%1s] >> %s\n",
                nseconds(ts) / 1E9,
                pthread_threadid_mpx(),
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

void LogPrint(const char *      tag,
        enum eLogLevel    level,
        const char *      func,
        size_t            line,
        const char *      format,
        ...) {
    va_list ap;
    char buf[1024];

    va_start(ap, format);
    vsnprintf(buf, 1024, format, ap);
    va_end(ap);

    static const char * LEVELS[] = {
        "F",
        "E",
        "W",
        "I",
        "D",
        NULL
    };

    struct timespec ts;
    absolute_time(&ts);
    char name[32];
    pthread_getname_mpx(pthread_self(), name, 32);
    if (name[0] == '\0') {
        fprintf(stdout, "[%08.03f][%07d][%-7.7s][%1s][%14.14s:%zu]\t: %s\n",
                nseconds(ts) / 1E9,
                mpx_gettid(),
                tag,
                LEVELS[level],
                func,
                line,
                buf);
    } else {
        fprintf(stdout, "[%08.03f][%-7.7s][%-7.7s][%1s][%14.14s:%zu]\t: %s\n",
                nseconds(ts) / 1E9,
                name,
                tag,
                LEVELS[level],
                func,
                line,
                buf);
    }

    if (level == LOG_FATAL) {
        BACKTRACE();
        __builtin_trap();
    }
}
