/******************************************************************************
 * Copyright (c) 2018, Chen Fang <mtdcy.chen@gmail.com>
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

// File:    SharedBuffer.h
// Author:  mtdcy.chen
// Changes:
//          1. 20181112     initial version
//

#ifndef _TOOLKIT_HEADERS_SHARED_BUFFER_H
#define _TOOLKIT_HEADERS_SHARED_BUFFER_H
#include <ABE/basic/Types.h>
#include <ABE/basic/Allocator.h>

#ifdef __cplusplus

__BEGIN_NAMESPACE_ABE

/**
 * a cow buffer
 * @note DON'T use sp to hold SharedBuffer
 */
struct __ABE_EXPORT SharedBuffer : protected SharedObject {
    protected:
        SharedBuffer();
        ~SharedBuffer();

    public:
        static SharedBuffer * Create(const Object<Allocator>&, size_t);
        void            deallocate();

    public:
        __ABE_INLINE SharedBuffer *  RetainBuffer()              { return (SharedBuffer *)SharedObject::RetainObject(); }
        size_t                       ReleaseBuffer(bool keep = false);
        __ABE_INLINE size_t          GetRetainCount() const      { return SharedObject::GetRetainCount();    }
        __ABE_INLINE bool            IsBufferShared() const      { return SharedObject::IsObjectShared();    }
        __ABE_INLINE bool            IsBufferNotShared() const   { return SharedObject::IsObjectNotShared(); }

    public:
        __ABE_INLINE char *          data()                      { return mData;                             }
        __ABE_INLINE const char *    data() const                { return mData;                             }
        __ABE_INLINE size_t          size() const                { return mSize;                             }

    public:
        SharedBuffer *  edit();
        SharedBuffer *  edit(size_t);

    private:
        Object<Allocator>   mAllocator;
        char *              mData;
        size_t              mSize;
};

__END_NAMESPACE_ABE

#endif   // __cplusplus

#ifdef __cplusplus
using __NAMESPACE_ABE::SharedBuffer;
#else
typedef struct SharedBuffer SharedBuffer;
#endif

__BEGIN_DECLS

/**
 * create a shared buffer & retain it
 * @param allocator     if allocator is NULL, default one will be used
 */
__ABE_EXPORT SharedBuffer *  SharedBufferCreate(Allocator * allocator, size_t);

/**
 * release this shared buffer
 */
__ABE_EXPORT void            SharedBufferRelease(SharedBuffer *);

/**
 * retain this shared buffer
 */
#define SharedBufferRetain(s)           (SharedBuffer *)SharedObjectRetain((SharedObject *)s)
#define SharedBufferGetRetainCount(s)   SharedObjectGetRetainCount((SharedObject *)s)

/**
 * is this shared buffer shared with others
 */
#define SharedBufferIsShared(s)         (SharedBufferGetRetainCount(s) > 1)
#define SharedBufferIsNotShared(s)      !SharedBufferIsShared(s)

/**
 * get shared buffer data pointer and size
 */
__ABE_EXPORT char *          SharedBufferGetData(const SharedBuffer *);
__ABE_EXPORT size_t          SharedBufferGetSize(const SharedBuffer *);

/**
 * make a shared buffer editable
 */
__ABE_EXPORT SharedBuffer *  SharedBufferEdit(SharedBuffer *);
__ABE_EXPORT SharedBuffer *  SharedBufferEditWithSize(SharedBuffer *, size_t);

/**
 * release a shared buffer without deallocate its memory
 * @return  return old ref count, if this is the last reference,
 *          deallocate its memory manually
 */
__ABE_EXPORT size_t          SharedBufferReleaseWithoutDeallocate(SharedBuffer *);
__ABE_EXPORT void            SharedBufferDeallocate(SharedBuffer *);

__END_DECLS

#endif // _TOOLKIT_HEADERS_SHARED_BUFFER_H
