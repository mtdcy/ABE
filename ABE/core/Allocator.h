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


// File:    Allocator.h
// Author:  mtdcy.chen
// Changes: 
//          1. 20160701     initial version
//

#ifndef ABE_ALLOCATOR_H
#define ABE_ALLOCATOR_H

#include <ABE/core/Types.h>

__BEGIN_NAMESPACE_ABE

struct ABE_EXPORT Allocator : public SharedObject {
    /**
     * create a system default allocator
     */
    static sp<Allocator> Default();
    static sp<Allocator> Create(UInt32 alignment);
    
    ABE_INLINE Allocator() : SharedObject(FOURCC('?mal')) { }
    
    /**
     * allocate n bytes memory
     * @return return Nil on OOM otherwise it must be success
     * @note alignment depends on the implementation
     */
    virtual void *  allocate(UInt32 n) = 0;
    /**
     * reallocate n bytes memory
     * @return return Nil on OOM otherwise it must be success
     * @note data will be kept
     */
    virtual void *  reallocate(void * p, UInt32 sz) = 0;
    /**
     * free the memory
     */
    virtual void    deallocate(void * p) = 0;
};

// for api compatible
#define kAllocatorDefault   Allocator::Default()
#define GetAllocator(n)     Allocator::Create(n)

__END_NAMESPACE_ABE

#endif // ABE_ALLOCATOR_H


