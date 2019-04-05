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


// File:    Types.h
// Author:  mtdcy.chen
// Changes: 
//          1. 20160701     initial version
//

#ifndef _TOOLKIT_HEADERS_TYPES_H
#define _TOOLKIT_HEADERS_TYPES_H

// DON'T include config.h here.
// only system wide macro allowed, like
// __ANDROID__ __ANDROID_API__
// __APPLE__
// __linux__ (including GNU/Linux and Android)
// refer to:
// https://sourceforge.net/p/predef/wiki/Architectures/
// https://sourceforge.net/p/predef/wiki/OperatingSystems/

#include <sys/types.h>      // all kinds of types define
#include <stdint.h>         // fixed width integer: int8_t int16_t int32_t ...
#include <stddef.h>         // size_t, null
#include <stdbool.h>        // true, false
#include <stdarg.h>         // va_list
#include <inttypes.h>       // for PRId32/PRId64/...
#include <sys/cdefs.h>      // others

// at least these types should be defined.
// uint8_t  int8_t
// uint16_t int16_t
// uint32_t int32_t
// uint64_t int64_t
// size_t ssize_t
// bool     true false
// null

#if defined(_WIN32) || defined(__MINGW32__)
#define __ABE_INLINE                __inline
#define __ABE_HIDDEN 
#define __ABE_EXPORT                __declspec(dllexport)
#else
//#define __ABE_INLINE                __attribute__ ((__always_inline__))
#define __ABE_INLINE                __attribute__ ((__visibility__("hidden"), __always_inline__))
#define __ABE_HIDDEN                __attribute__ ((__visibility__("hidden")))
#define __ABE_EXPORT                __attribute__ ((__visibility__("default")))
#define __ABE_DEPRECATED            __attribute__ ((deprecated))
#endif

#ifndef REMOVE_ATOMICS
#ifndef ATOMIC_MEMMODEL
#define ATOMIC_MEMMODEL __ATOMIC_SEQ_CST
#endif

// return old value on macro
#define atomic_store(ptr, val)  __atomic_exchange_n(ptr, val, ATOMIC_MEMMODEL)
#define atomic_load(ptr)        __atomic_load_n(ptr, ATOMIC_MEMMODEL)
#define atomic_add(ptr, val)    __atomic_fetch_add(ptr, val, ATOMIC_MEMMODEL)
#define atomic_sub(ptr, val)    __atomic_fetch_sub(ptr, val, ATOMIC_MEMMODEL)
#define atomic_and(ptr, val)    __atomic_fetch_and(ptr, val, ATOMIC_MEMMODEL)
#define atomic_or(ptr, val)     __atomic_fetch_or(ptr, val, ATOMIC_MEMMODEL)
#define atomic_xor(ptr, val)    __atomic_fetch_xor(ptr, val, ATOMIC_MEMMODEL)
#define atomic_nand(ptr, val)   __atomic_fetch_nand(ptr, val, ATOMIC_MEMMODEL)


// return true on success
//  if *p0 == *p1: val => *p0 : return true
//  else: *p0 => *p1 : return false
#define atomic_compare_exchange(p0, p1, val) \
    __atomic_compare_exchange_n(p0, p1, val, false, ATOMIC_MEMMODEL, ATOMIC_MEMMODEL)
// fence
#define atomic_fence()          __atomic_thread_fence(ATOMIC_MEMMODEL)
#endif

#ifdef __cplusplus
#define __BEGIN_NAMESPACE(x)            namespace x {
#define __END_NAMESPACE(x)              }
#define __USING_NAMESPACE(x)            using namespace x;

#define __NAMESPACE_ABE                 abe
#define __BEGIN_NAMESPACE_ABE           __BEGIN_NAMESPACE(__NAMESPACE_ABE)
#define __END_NAMESPACE_ABE             __END_NAMESPACE(__NAMESPACE_ABE)

#define __NAMESPACE_ABE_PRIVATE         abe_private
#define __BEGIN_NAMESPACE_ABE_PRIVATE   __BEGIN_NAMESPACE(__NAMESPACE_ABE) __BEGIN_NAMESPACE(__NAMESPACE_ABE_PRIVATE)
#define __END_NAMESPACE_ABE_PRIVATE     __END_NAMESPACE(__NAMESPACE_ABE_PRIVATE) __END_NAMESPACE(__NAMESPACE_ABE)

#define USING_NAMESPACE_ABE             __USING_NAMESPACE(__NAMESPACE_ABE)
#define USING_NAMESPACE_ABE_PRIVATE     __USING_NAMESPACE(__NAMESPACE_ABE::__NAMESPACE_ABE_PRIVATE)

// borrow from Android
// Put this in the private: declarations for a class to be uncopyable.
#define DISALLOW_COPY(TypeName) TypeName(const TypeName&)

// Put this in the private: declarations for a class to be unassignable.
#define DISALLOW_ASSIGN(TypeName) void operator=(const TypeName&)

// A macro to disallow the copy constructor and operator= functions
// This should be used in the private: declarations for a class
// In most case, copy constructor and operator= is not neccessary, 
// and it may cause problem if you don't declare it or misuse it.
#define DISALLOW_EVILS(TypeName)                \
    TypeName(const TypeName&);                  \
    TypeName& operator=(const TypeName&)

#endif // __cplusplus

#include <errno.h>
#ifdef __cplusplus
// refer to android@system/core/include/utils/Errors.h
__BEGIN_NAMESPACE_ABE
#endif

/*
 * Error codes.
 * All error codes are negative values.
 */

// Win32 #defines NO_ERROR as well.  It has the same value, so there's no
// real conflict, though it's a bit awkward.
#ifdef _WIN32
# undef NO_ERROR
#endif

enum status_t {
    OK                  = 0,    // Everything's swell.
    NO_ERROR            = 0,    // No errors.

    UNKNOWN_ERROR       = (-2147483647-1), // INT32_MIN value

    TRY_AGAIN           = -EAGAIN,
    NO_MEMORY           = -ENOMEM,
    INVALID_OPERATION   = -ENOSYS,
    BAD_VALUE           = -EINVAL,
    BAD_TYPE            = (UNKNOWN_ERROR + 1),
    NAME_NOT_FOUND      = -ENOENT,
    PERMISSION_DENIED   = -EPERM,
    NO_INIT             = -ENODEV,
    ALREADY_EXISTS      = -EEXIST,
    DEAD_OBJECT         = -EPIPE,
    FAILED_TRANSACTION  = (UNKNOWN_ERROR + 2),
    JPARKS_BROKE_IT     = -EPIPE,
#if !defined(HAVE_MS_C_RUNTIME)
    BAD_INDEX           = -EOVERFLOW,
    NOT_ENOUGH_DATA     = -ENODATA,
    WOULD_BLOCK         = -EWOULDBLOCK,
    TIMED_OUT           = -ETIMEDOUT,
    UNKNOWN_TRANSACTION = -EBADMSG,
#else
    BAD_INDEX           = -E2BIG,
    NOT_ENOUGH_DATA     = (UNKNOWN_ERROR + 3),
    WOULD_BLOCK         = (UNKNOWN_ERROR + 4),
    TIMED_OUT           = (UNKNOWN_ERROR + 5),
    UNKNOWN_TRANSACTION = (UNKNOWN_ERROR + 6),
#endif
    FDS_NOT_ALLOWED     = (UNKNOWN_ERROR + 7),
};

#ifdef __cplusplus
__END_NAMESPACE_ABE
#endif // __cplusplus

#endif // _TOOLKIT_HEADERS_TYPES_H
