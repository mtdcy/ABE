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


// File:    STL.h
// Author:  mtdcy.chen
// Changes: 
//          1. 20160829     initial version
//
// This File should only be used by container implementation

#ifndef _TOOLKIT_HEADERS_STL_H
#define _TOOLKIT_HEADERS_STL_H 

#ifdef __cplusplus
#include <ABE/stl/Traits.h>
#include <new>
#include <string.h>     // memcpy & memmove
#include <math.h>       // abs
__BEGIN_NAMESPACE_ABE

// TYPE()
typedef void (*type_construct_t)(void *storage, size_t);
// ~TYPE()
typedef void (*type_destruct_t)(void *storage, size_t);
// TYPE(const TYPE&);
typedef void (*type_copy_t)(void *storage, const void *, size_t);
// move TYPE
typedef void (*type_move_t)(void *dest, void *src, size_t);
// operator< or operator> or operator==
typedef bool (*type_compare_t)(const void* lhs, const void* rhs);

//////////////////////////////////////////////////////////////////////////////
// templates for type_helper
template <typename TYPE> static __ABE_INLINE void type_construct(void *storage, size_t n) {
    if (is_trivial_ctor<TYPE>::value) {
        // NOTHING
    } else {
        TYPE *p = static_cast<TYPE*>(storage);
        while (n--) { new (p++) TYPE; }
    }
}

//////////////////////////////////////////////////////////////////////////////
template <typename TYPE> static __ABE_INLINE void type_destruct(void *storage, size_t n) {
    if (is_trivial_dtor<TYPE>::value) {
        // NOTHING
    } else {
        TYPE *p = static_cast<TYPE*>(storage);
        while (n--) { p->~TYPE(); ++p; }
    }
}

//////////////////////////////////////////////////////////////////////////////
template <typename TYPE> static __ABE_INLINE void type_copy(void *storage, const void *_from, size_t n) {
    if (is_trivial_copy<TYPE>::value) {
        memcpy(storage, _from, sizeof(TYPE) * n);
    } else {
        TYPE *p = static_cast<TYPE*>(storage);
        const TYPE *from = static_cast<const TYPE*>(_from);
        while (n--) { new (p++) TYPE(*from++); }
    }
}

//////////////////////////////////////////////////////////////////////////////
template <typename TYPE> static __ABE_INLINE void type_move(void * _dest, void * _src, size_t n) {
    TYPE * dest = (TYPE *)_dest;
    TYPE * src  = (TYPE *)_src;
    if (is_trivial_move<TYPE>::value) {
        // memmove can handle memory overlap
        memmove(dest, src, n * sizeof(TYPE));
    } else if (abs(dest - src) > n) {
        // no overlap
        type_copy<TYPE>(dest, src, n);
        type_destruct<TYPE>(src, n);
    } else {
        if (dest > src) {
            dest    += n;
            src     += n;
            while (n--) {
                dest    -= 1;
                src     -= 1;
                type_copy<TYPE>(dest, src, 1);
                type_destruct<TYPE>(src, 1);
            }
        } else {
            while (n--) {
                type_copy<TYPE>(dest, src, 1);
                type_destruct<TYPE>(src, 1);
                dest    += 1;
                src     += 1;
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////////
// for template wrapper => implementation
struct __ABE_HIDDEN TypeHelper {
    private:
        size_t              type_size;
        type_construct_t    construct;
        type_destruct_t     destruct;
        type_copy_t         copy;
        type_move_t         move;

    public:
        __ABE_INLINE TypeHelper(size_t _size,
                type_construct_t _ctor,
                type_destruct_t _dtor,
                type_copy_t _copy = NULL,
                type_move_t _move = NULL) :
            type_size(_size), construct(_ctor), destruct(_dtor), copy(_copy), move(_move) { }

        __ABE_INLINE size_t size() const                                         { return type_size;         }
        __ABE_INLINE void do_construct(void * storage, size_t n)                 { construct(storage, n);    }
        __ABE_INLINE void do_destruct(void * storage, size_t n)                  { destruct(storage, n);     }
        __ABE_INLINE void do_copy(void * storage, const void * from, size_t n)   { copy(storage, from, n);   }
        __ABE_INLINE void do_move(void * dest, void * src, size_t n)             { move(dest, src, n);       }
};

// using template partial specilization
#define Helper(WHO, WHAT)                                                   \
    template <typename TYPE, bool ENABLE> struct __ABE_HIDDEN WHO##_helper; \
    template <typename TYPE> struct WHO##_helper<TYPE, true> {              \
        WHAT get(void) const { return WHO<TYPE>; }                          \
    };                                                                      \
    template <typename TYPE> struct WHO##_helper<TYPE, false> {             \
        WHAT get(void) const { return NULL; }                               \
    };

Helper(type_construct, type_construct_t);
Helper(type_destruct, type_destruct_t);
Helper(type_copy, type_copy_t);
Helper(type_move, type_move_t);

template <typename TYPE, bool CTOR, bool COPY, bool MOVE>
static __ABE_HIDDEN TypeHelper TypeHelperBuilder() {
    return TypeHelper(sizeof(TYPE),
            type_construct_helper<TYPE, CTOR>().get(),
            type_destruct_helper<TYPE, true>().get(),
            type_copy_helper<TYPE, COPY>().get(),
            type_move_helper<TYPE, MOVE>().get());
}
#undef Helper

//////////////////////////////////////////////////////////////////////////////
template <typename TYPE> static __ABE_INLINE bool type_compare_less(const void* lhs, const void* rhs) {
    return *static_cast<const TYPE*>(lhs) < *static_cast<const TYPE*>(rhs);
}

template <typename TYPE> static __ABE_INLINE bool type_compare_more(const void* lhs, const void* rhs) {
    return *static_cast<const TYPE*>(lhs) > *static_cast<const TYPE*>(rhs);
}

template <typename TYPE> static __ABE_INLINE bool type_compare_equal(const void* lhs, const void* rhs) {
    return *static_cast<const TYPE*>(lhs) == *static_cast<const TYPE*>(rhs);
}

__END_NAMESPACE_ABE
#endif // __cplusplus
#endif // _TOOLKIT_HEADERS_STL_H 
