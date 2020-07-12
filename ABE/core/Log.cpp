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

#include "System.h"
#include "compat/pthread.h"

#if defined(_WIN32) || defined(__MINGW32__)
#define PRIpid_t    "lld"
#else
#define PRIpid_t    "d"
#endif

USING_NAMESPACE_ABE

BEGIN_DECLS

// NOTE:
//  1. should avoid having dependency to others as possible as you can
//  2. must avoid calling others which uses Log.

static Bool __init = False;
static ABE_INLINE void _init() {
#if defined(_WIN32) || defined(__MINGW32__)
    setvbuf(stdout, 0, _IOLBF, 2048);
#endif
    __init = True;
}

typedef void (*Callback_t)(const Char *);
static Callback_t   __callback = Nil;

void LogSetCallback(Callback_t cb) { __callback = cb; }

void SystemLogPrint(const Char *      tag,
        enum eLogLevel    level,
        const Char *      func,
        UInt32            line,
        const Char *      format,
        ...) {

    if (__builtin_expect(__init == False, False)) _init();

    va_list ap;
    Char buf0[1024];
    Char buf1[1024];

    va_start(ap, format);
    vsnprintf(buf0, 1024, format, ap);
    va_end(ap);

    static const Char * LEVELS[] = {
        "F",
        "E",
        "W",
        "I",
        "D",
        Nil
    };

    Float64 ts = Time::Now(True).seconds();
    
    Char name[16];
    pthread_getname(name, 16);

    snprintf(buf1, 1024, "[%.06f][%-7.7s][%-7.7s][%1s][%14.14s:%u] : %s\n",
            ts,
            name,
            tag,
            LEVELS[level],
            func,
            line,
            buf0);
    
    if (__callback) {
        __callback(buf1);
    } else {
        fprintf(stdout, "%s", buf1);
#if defined(_WIN32) || defined(__MINGW32__)
        fflush(stdout);
#endif
    }

    if (level == LOG_FATAL) {
        CallStackPrint();
        __builtin_trap();
    }
}

END_DECLS
