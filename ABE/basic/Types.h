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

#ifndef ABE_HEADERS_TYPES_H
#define ABE_HEADERS_TYPES_H

/**
 * @note NO config.h for ABE public headers
 *
 * @note only system/libc/compiler macro allowed, like __APPLE__
 *  https://sourceforge.net/p/predef/wiki/Architectures/
 *  https://sourceforge.net/p/predef/wiki/Compilers/
 *  https://sourceforge.net/p/predef/wiki/OperatingSystems/
 *  https://blog.kowalczyk.info/article/j/guide-to-predefined-macros-in-c-compilers-gcc-clang-msvc-etc..html
 *
 * Compiler:
 *    _MSC_VER & __clang__ & __GNUC__
 * Platform:
 *    __APPLE__ & _WIN32 & _WIN64 & __MINGW32__ & __ANDROID__ & __linux__
 *
 *
 * @note we assume everything in bytes-order
 * @note NO Endian specific operations expect tool designed for this
 **/

#include <stdint.h>         // fixed width integer: int8_t int16_t int32_t ...
#include <stddef.h>         // size_t, null
#include <stdbool.h>        // true, false
#include <stdarg.h>         // va_list
#include <sys/types.h>      // other types: ssize_t
#include <inttypes.h>       // for PRId32/PRId64/...

// at least these types should be defined.
// uint8_t  int8_t
// uint16_t int16_t
// uint32_t int32_t
// uint64_t int64_t
// size_t ssize_t
// bool     true false
// null

#if !defined(__BEGIN_DECLS) || !defined(__END_DECLS)
#ifdef __cplusplus
#define __BEGIN_DECLS           extern "C" {
#define __END_DECLS             }
#else
#define __BEGIN_DECLS
#define __END_DECLS
#endif
#endif

#if defined(_MSC_VER)
#define ABE_INLINE              inline
#ifdef BUILD_ABE_DLL
#define ABE_EXPORT              __declspec(dllexport)
#else
#define ABE_EXPORT              __declspec(dllimport)
#endif
#define ABE_DEPRECATED          __declspec(deprecated)
#else
//#define __ABE_INLINE          __attribute__ ((__always_inline__))
#define ABE_INLINE              __attribute__ ((__visibility__("hidden"), __always_inline__)) inline
#define ABE_EXPORT              __attribute__ ((__visibility__("default")))
#define ABE_DEPRECATED          __attribute__ ((deprecated))
#endif

#if defined(_MSC_VER)
#define ABE_LIKELY(x)           (x)
#define ABE_UNLIKELY(x)         (x)
#else
#define ABE_LIKELY(x)           __builtin_expect(x, true)
#define ABE_UNLIKELY(x)         __builtin_expect(x, false)
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
#define DISALLOW_COPY(TypeName)         TypeName(const TypeName&)

// Put this in the private: declarations for a class to be unassignable.
#define DISALLOW_ASSIGN(TypeName)       void operator=(const TypeName&)

// A macro to disallow the copy constructor and operator= functions
// This should be used in the private: declarations for a class
// In most case, copy constructor and operator= is not neccessary,
// and it may cause problem if you don't declare it or misuse it.
#define DISALLOW_EVILS(TypeName)            \
    private:                                \
    TypeName(const TypeName&);              \
    TypeName& operator=(const TypeName&)

#define DISALLOW_DYNAMIC(TypeName)          \
    private:                                \
    void *  operator new(size_t);           \
    void    operator delete(void *)
#endif // __cplusplus

#include <errno.h>
// make sure we share the same errorno on all platforms
enum status_t {
    OK                  = 0,
    ERROR_UNKNOWN       = INT32_MIN,
    ERROR_AGAIN         = -EAGAIN,      ///< try again, @see EAGAIN
    ERROR_OOM           = -ENOMEM,      ///< out of memory, @see ENOMEM
    ERROR_INVALID       = -EINVAL,      ///< invalid argument or operation, @see EINVAL, @see ENOSYS
    ERROR_DENIED        = -EPERM,       ///< permission denied, @see EPERM
    ERROR_TIMEOUT       = -ETIMEDOUT,   ///< time out, @see ETIMEDOUT
    ERROR_DEADLOCK      = -EDEADLK,     ///< dead lock, @see EDEADLK, @see EWOULDBLOCK
    ERROR_BUSY          = -EBUSY,       ///< device or resource is busy, @see EBUSY
    ERROR_EXIST         = -EEXIST,      ///< file or resource is exists, @see EEXIST
};

#endif // ABE_HEADERS_TYPES_H

