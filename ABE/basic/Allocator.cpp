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


// File:    Allocator.h
// Author:  mtdcy.chen
// Changes: 
//          1. 20160701     initial version
//

#define LOG_TAG "Allocator"
#include "Log.h"

#include "Allocator.h" 
#include <stdlib.h>
#include <string.h>

#define MIN(a, b)   (a) > (b) ? (b) : (a)
#define POW_2(x)    (1 << (32 - __builtin_clz((x)-1)))
#define ALIGN (32)

static int posix_memalign_mpx(void **ptr, size_t alignment, size_t size) {
#ifdef HAVE_POSIX_MEMALIGN
    return posix_memalign(ptr, alignment, size);
#else
    if (alignment < ALIGN) alignment = ALIGN;

    *ptr = NULL;

#if defined(HAVE_MEMALIGN)
#ifndef __DJGPP__
    *ptr = memalign(alignment, size);
#else
    *ptr = memalign(size, alignment);
#endif
    L1_CHECK_NULL(*ptr);
    return 0;
#else
    // FIXME
    *ptr = malloc(size);
    return 0;
#endif
#endif // HAVE_POSIX_MEMALIGN
}

__BEGIN_NAMESPACE_ABE

struct AllocatorDefault : public Allocator {
    AllocatorDefault() : Allocator() { }
    virtual ~AllocatorDefault() { }
    virtual void * allocate(size_t size) {
        void * ptr = malloc(size);
        CHECK_NULL(ptr);
        return ptr;
    }
    virtual void * reallocate(void * ptr, size_t size) {
        void * _ptr = realloc(ptr, size);
        CHECK_NULL(_ptr);
        return _ptr;
    }
    virtual void deallocate(void * ptr) {
        CHECK_NULL(ptr);
        free(ptr);
    }
};
Object<Allocator> kAllocatorDefault = new AllocatorDefault;

struct AllocatorDefaultAligned : public Allocator {
    const size_t mAlignment;
    size_t mSize;
    AllocatorDefaultAligned(size_t align) : Allocator(), mAlignment(POW_2(align)) { }
    virtual ~AllocatorDefaultAligned() { }
    virtual void * allocate(size_t size) {
        void * ptr;
        CHECK_EQ(posix_memalign_mpx(&ptr, mAlignment, size), 0);
        CHECK_NULL(ptr);
        mSize = size;
        return ptr;
    }
    virtual void * reallocate(void * ptr, size_t size) {
        // From man(3) posix_memalign:
        // Memory that is allocated via posix_memalign() can be used
        // as an argument in subsequent calls to realloc(3), reallocf(3),
        // and free(3).  (Note however, that the allocation returned
        // by realloc(3) or reallocf(3) is not guaranteed to preserve
        // the original alignment).
        void * _ptr;
        CHECK_EQ(posix_memalign_mpx(&_ptr, mAlignment, size), 0);
        CHECK_NULL(_ptr);
        memcpy(_ptr, ptr, MIN(size, mSize));
        mSize = size;
        free(ptr);
        return _ptr;
    }
    virtual void deallocate(void * ptr) {
        CHECK_NULL(ptr);
        free(ptr);
        mSize = 0;
    }
};

Object<Allocator> GetAlignedAllocator(size_t alignment) {
    return new AllocatorDefaultAligned(alignment);
}

__END_NAMESPACE_ABE

extern "C" {
    //using mtdcy::Allocator;

    Allocator * AllocatorGetDefault(void) {
        Allocator * shared = __NAMESPACE_ABE::kAllocatorDefault.get();
        return static_cast<Allocator*>(shared->RetainObject());
    }

    Allocator * AllocatorGetDefaultAligned(size_t alignment) {
        Allocator * shared = new __NAMESPACE_ABE::AllocatorDefaultAligned(alignment);
        return static_cast<Allocator*>(shared->RetainObject());
    }

    void * AllocatorAllocate(Allocator * shared, size_t n) {
        return shared->allocate(n);
    }

    void * AllocatorReallocate(Allocator * shared, void * p, size_t n) {
        return shared->reallocate(p, n);
    }

    void AllocatorDeallocate(Allocator * shared, void * p) {
        shared->deallocate(p);
    }
}
