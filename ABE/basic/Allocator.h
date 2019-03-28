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

#ifndef _TOOLKIT_HEADERS_ALLOCATOR_H
#define _TOOLKIT_HEADERS_ALLOCATOR_H

#include <ABE/basic/Types.h>
#include <ABE/basic/SharedObject.h>

#ifdef __cplusplus

__BEGIN_NAMESPACE_ABE

struct Allocator : public SharedObject {
    Allocator() : SharedObject(OBJECT_ID_ALLOCATOR) { }
    virtual ~Allocator() { }
    virtual void *  allocate(size_t size) = 0;
    virtual void *  reallocate(void * ptr, size_t size) = 0;
    virtual void    deallocate(void * ptr) = 0;
};
extern sp<Allocator> kAllocatorDefault;
sp<Allocator> GetAlignedAllocator(size_t alignment);

__END_NAMESPACE_ABE
#endif   // __cplusplus

#ifdef __cplusplus
using __NAMESPACE_ABE::Allocator;
#else
typedef struct Allocator Allocator;
#endif

__BEGIN_DECLS

/**
 * retain default allocator
 */
Allocator *     AllocatorGetDefault(void);
Allocator *     AllocatorGetDefaultAligned(size_t);

/**
 * release a allocator
 */
#define AllocatorRelease(s)         SharedObjectRelease((SharedObject *)s)

/**
 * retain a allocator
 */
#define AllocatorRetain(s)          (Allocator *)SharedObjectRetain((SharedObject *)s)
#define AllocatorGetRetainCount(s)  SharedObjectGetRetainCount((SharedObject *)s)

/**
 * get allocator alignment
 */
size_t          AllocatorGetAlignment(Allocator *);
/**
 * allocate memory using allocator
 */
void *          AllocatorAllocate(Allocator *, size_t);
void *          AllocatorReallocate(Allocator *, void *, size_t);
/**
 * free memory using allocator
 */
void            AllocatorDeallocate(Allocator *, void *);
__END_DECLS

#endif // _TOOLKIT_HEADERS_ALLOCATOR_H


