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

#ifndef ABE_HEADERS_STL_H
#define ABE_HEADERS_STL_H 

#include <ABE/stl/Traits.h>
#include <new>
#include <string.h>     // memcpy & memmove
#include <math.h>
__BEGIN_NAMESPACE_ABE

// TYPE()
typedef void (*type_construct_t)(void *storage, UInt32);
// ~TYPE()
typedef void (*type_destruct_t)(void *storage, UInt32);
// TYPE(const TYPE&);
typedef void (*type_copy_t)(void *storage, const void *, UInt32);
// move TYPE
typedef void (*type_move_t)(void *dest, void *src, UInt32);
// operator< or operator> or operator==
typedef Bool (*type_compare_t)(const void* lhs, const void* rhs);

//////////////////////////////////////////////////////////////////////////////
// templates for type_helper
template <typename TYPE> static ABE_INLINE void type_construct(void *storage, UInt32 n) {
    if (is_trivial_ctor<TYPE>::value) {
        // NOTHING
    } else {
        TYPE *p = static_cast<TYPE*>(storage);
        while (n--) { ::new (p++) TYPE; }
    }
}

//////////////////////////////////////////////////////////////////////////////
template <typename TYPE> static ABE_INLINE void type_destruct(void *storage, UInt32 n) {
    if (is_trivial_dtor<TYPE>::value) {
        // NOTHING
    } else {
        TYPE *p = static_cast<TYPE*>(storage);
        while (n--) { p->~TYPE(); ++p; }
    }
}

//////////////////////////////////////////////////////////////////////////////
template <typename TYPE> static ABE_INLINE void type_copy_trivial(void * storage, const void * from, UInt32 n) {
    memcpy(storage, from, n * sizeof(TYPE));
}

template <typename TYPE> static ABE_INLINE void type_copy(void *storage, const void *_from, UInt32 n) {
    TYPE *p = static_cast<TYPE*>(storage);
    const TYPE *from = static_cast<const TYPE*>(_from);
    while (n--) { ::new (p++) TYPE(*from++); }
}

//////////////////////////////////////////////////////////////////////////////
template <typename TYPE> static ABE_INLINE void type_move_trivial(void * dest, void * src, UInt32 n) {
    memmove(dest, src, n * sizeof(TYPE));
}

template <typename TYPE> static ABE_INLINE void type_move(void * _dest, void * _src, UInt32 n) {
    TYPE * dest = (TYPE *)_dest;
    TYPE * src  = (TYPE *)_src;
    if ((UInt32)abs(dest - src) > n) {
        // no overlap
        if (is_trivial_copy<TYPE>::value) 
            type_copy_trivial<TYPE>(dest, src, n);
        else
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
struct TypeHelper : public StaticObject {
    private:
        UInt32              type_size;
        type_construct_t    construct;
        type_destruct_t     destruct;
        type_copy_t         copy;
        type_move_t         move;

    public:
        ABE_INLINE TypeHelper(UInt32 _size,
                type_construct_t _ctor,
                type_destruct_t _dtor,
                type_copy_t _copy = Nil,
                type_move_t _move = Nil) :
            type_size(_size), construct(_ctor), destruct(_dtor), copy(_copy), move(_move) { }

        ABE_INLINE UInt32 size() const                                         { return type_size;         }
        ABE_INLINE void do_construct(void * storage, UInt32 n)                 { construct(storage, n);    }
        ABE_INLINE void do_destruct(void * storage, UInt32 n)                  { destruct(storage, n);     }
        ABE_INLINE void do_copy(void * storage, const void * from, UInt32 n)   { copy(storage, from, n);   }
        ABE_INLINE void do_move(void * dest, void * src, UInt32 n)             { move(dest, src, n);       }
};

// using template partial specilization
#define Helper(WHAT, TH0, TH1, TRIVIAL)                                         \
    template <typename TYPE, Bool trivail = TRIVIAL<TYPE>::value>               \
    struct WHAT##_helper_;                                         \
    template <typename TYPE> struct WHAT##_helper_<TYPE, False> {               \
        WHAT operator()(void) { return TH0<TYPE>; }                             \
    };                                                                          \
    template <typename TYPE> struct WHAT##_helper_<TYPE, True> {                \
        WHAT operator()(void) { return TH1<TYPE>; }                             \
    };                                                                          \
    template <typename TYPE, Bool ENABLE> struct WHAT##_helper;    \
    template <typename TYPE> struct WHAT##_helper<TYPE, True> {                 \
        WHAT get(void) const { return WHAT##_helper_<TYPE>()(); }               \
    };                                                                          \
    template <typename TYPE> struct WHAT##_helper<TYPE, False> {                \
        WHAT get(void) const { return Nil; }                                   \
    };

Helper(type_construct_t, type_construct, type_construct, is_trivial_ctor);
Helper(type_destruct_t, type_destruct, type_destruct, is_trivial_dtor);
Helper(type_copy_t, type_copy, type_copy_trivial, is_trivial_copy);
Helper(type_move_t, type_move, type_move_trivial, is_trivial_move);

template <typename TYPE, Bool CTOR, Bool COPY, Bool MOVE>
static TypeHelper TypeHelperBuilder() {
    return TypeHelper(sizeof(TYPE),
            type_construct_t_helper<TYPE, CTOR>().get(),
            type_destruct_t_helper<TYPE, True>().get(),
            type_copy_t_helper<TYPE, COPY>().get(),
            type_move_t_helper<TYPE, MOVE>().get());
}
#undef Helper

//////////////////////////////////////////////////////////////////////////////
template <typename TYPE> static ABE_INLINE Bool type_compare_less(const void* lhs, const void* rhs) {
    return *static_cast<const TYPE*>(lhs) < *static_cast<const TYPE*>(rhs);
}

template <typename TYPE> static ABE_INLINE Bool type_compare_more(const void* lhs, const void* rhs) {
    return *static_cast<const TYPE*>(lhs) > *static_cast<const TYPE*>(rhs);
}

template <typename TYPE> static ABE_INLINE Bool type_compare_equal(const void* lhs, const void* rhs) {
    return *static_cast<const TYPE*>(lhs) == *static_cast<const TYPE*>(rhs);
}

__END_NAMESPACE_ABE

#endif // ABE_HEADERS_STL_H 
