/******************************************************************************
 * Copyright (c) 2020, Chen Fang <mtdcy.chen@gmail.com>
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


// File:    CallStack.cpp
// Author:  mtdcy.chen
// Changes: 
//          1. 20161012     initial version
//
#define _DARWIN_C_SOURCE    // for macOS
#define _GNU_SOURCE 

#define LOG_TAG "backtrace"
#include "ABE/core/Log.h"

#include "System.h"

#include <stdio.h>
#include <stdlib.h>

#include <cxxabi.h>

#if 0
#include <execinfo.h>
#include <stdlib.h>
#else 
#ifdef __MINGW32__
#else
#define USE_UNWIND   1
#endif
#endif

#define MAX_STACK 32

USING_NAMESPACE_ABE

__BEGIN_DECLS

#ifdef USE_UNWIND 
#include <unwind.h>
#include <dlfcn.h>
// borrow from android:bionic
struct stack_crawl_state_t {
    UInt64*         frames;
    UInt32          frame_count;
    UInt32          cur_frame;
};

static _Unwind_Reason_Code trace_function(struct _Unwind_Context* context, void* arg) {
    struct stack_crawl_state_t* state = (struct stack_crawl_state_t*)(arg);

    uintptr_t ip = _Unwind_GetIP(context);

    // The instruction pointer is pointing at the instruction after the return
    // call on all architectures.
    // Modify the pc to point at the real function.
    if (ip != 0) {
#if defined(__arm__)
        // If the ip is suspiciously low, do nothing to avoid a segfault trying
        // to access this memory.
        if (ip >= 4096) {
            // Check bits [15:11] of the first halfword assuming the instruction
            // is 32 bits long. If the bits are any of these values, then our
            // assumption was correct:
            //  b11101
            //  b11110
            //  b11111
            // Otherwise, this is a 16 bit instruction.
            UInt16 value = (*(UInt16*)(ip - 2)) >> 11;
            if (value == 0x1f || value == 0x1e || value == 0x1d) {
                ip -= 4;
            } else {
                ip -= 2;
            }
        }
#elif defined(__aarch64__)
        // All instructions are 4 bytes long, skip back one instruction.
        ip -= 4;
#elif defined(__i386__) || defined(__x86_64__)
        // It's difficult to decode exactly where the previous instruction is,
        // so subtract 1 to estimate where the instruction lives.
        ip--;
#endif

#if 0
        // Do not record the frames that fall in our own shared library.
        if (g_current_code_map && (ip >= g_current_code_map->start) && ip < g_current_code_map->end) {
            return _URC_NO_REASON;
        }
#endif
    }

    state->frames[state->cur_frame++] = (UInt64)ip;
    return (state->cur_frame >= state->frame_count) ? _URC_END_OF_STACK : _URC_NO_REASON;
}
#endif

// save current call stack
UInt32 CallStackGet(UInt64 array[], UInt32 max) {
#ifdef USE_UNWIND
    UInt64 stack[MAX_STACK];
    struct stack_crawl_state_t state;
    state.frames      = stack;
    state.frame_count = MAX_STACK;
    state.cur_frame   = 0;
    _Unwind_Backtrace(trace_function, &state);
    for (UInt32 i = 1; i < state.cur_frame && i - 1 < max; ++i) {
        array[i - 1] = stack[i];
    }
    // remove only first pc
    return state.cur_frame - 1;
#elif defined(__MINGW32__)
    return 0;
#else
    UInt64 stack[MAX_STACK];
    UInt32 n = backtrace(stack, MAX_STACK);
    for (UInt32 i = 1; i < n - 1 && i - 1 < max; ++i) {
        array[i-1]  = stack[i];
    }
    // remove first ip and last pc
    return n - 1 - 1;
#endif
}

#ifdef __LP64__
#define PRIxp   "016x"
#else
#define PRIxp   "08x"
#endif

// print this call stack
void CallStackPut(const UInt64 array[], UInt32 size) {
#ifdef USE_UNWIND
    for (UInt32 i = 0; i < size; ++i) {
        const void *addr = (const void *)array[i];

        // dladdr can only see functions exported in the dynamic symbol table
        // local symbol like hidden visibility, static, can not see by dladdr
        Dl_info info;
        if (dladdr(addr, &info) && info.dli_fname) {
            if (info.dli_sname) {
#ifdef __APPLE__
                // VERY SLOW
                Char cmd[1024];
                snprintf(cmd, 1024, "atos -o %s -l %p %p",
                         info.dli_fname, info.dli_fbase, addr);
                system(cmd);
#else
#if 1
                // TODO: line number
                INFO("%02d [%p]: %s:%s+%d",
                     i, addr,
                     info.dli_fname,
                     ::abi::__cxa_demangle(info.dli_sname, 0, 0, 0),
                     //info.dli_sname,
                     (const Char *)addr - (Char *)info.dli_saddr);
#else
                INFO("%02d: %" PRIxp "+%" PRIxp " -> %s:%s:%d",
                     i,
                     info.dli_fbase, addr,
                     info.dli_fname, info.dli_sname,
                     (Int)((const Char *)addr - (Char *)info.dli_saddr));
#endif
#endif
            } else {
                INFO("%02d [%p]: %s",
                        i, addr, info.dli_fname);
            }
        } else {
            INFO("%02d: unknown", i);
        }
    }
#elif defined(__MINGW32__)
    // NOTHING
#else
    // backtrace_symbols uses malloc, if malloc and free been overrided,
    // it is a problem.
    Char **lines = backtrace_symbols(array, size);
    for (UInt32 i = 0; i < size; ++i) {
        INFO("%s", lines[i]);
    }
    free(lines);
#endif
}

__END_DECLS
