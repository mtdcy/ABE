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


// File:    SharedObject.cpp
// Author:  mtdcy.chen
// Changes: 
//          1. 20181112     initial version
//

#define LOG_TAG "SharedObject"
//#define LOG_NDEBUG 0
#include "Log.h"
#include "SharedBuffer.h"

#include <string.h>
#include <stdlib.h>
#include <new>

#define MIN(a, b) ((a) > (b) ? (b) : (a))

// we can not use CHECK_EQ/... here, which use String
#define FATAL_CHECK(a, b, op) do {                                  \
    if (__builtin_expect(!((a) op (b)), 0)) {                       \
        FATAL(__FILE__ ":" LITERAL_TO_STRING(__LINE__)              \
                " CHECK(" #a " " #op " " #b ") failed. ");          \
    }                                                               \
} while(0)

#define FATAL_CHECK_EQ(a, b)    FATAL_CHECK(a, b, ==)
#define FATAL_CHECK_GE(a, b)    FATAL_CHECK(a, b, >=)
#define FATAL_CHECK_GT(a, b)    FATAL_CHECK(a, b, >)

#define ID_UNKNOWN  0xffffffff

__BEGIN_NAMESPACE_ABE

SharedObject::SharedObject(const UInt32 id) : mID(id), mRefs(0) {
}

SharedObject *  SharedObject::RetainObject() {
    DEBUG("retain %" PRIu32, mID);
    UInt32 refs = ++mRefs;
    if (refs == 1) onFirstRetain();
    return this;
}

UInt32 SharedObject::ReleaseObject(Bool keep) {
    DEBUG("release %" PRIu32, mID);
    if (mRefs.load() == 0) {
        ERROR("release object %p after destruction", this);
        return 0;
    }
    
    UInt32 refs = --mRefs;
    if (refs == 0) {
        onLastRetain();
        if (!keep) delete this;
    }
    return (UInt32)refs;
}

SharedObject::SharedObject(const SharedObject& rhs) : mID(rhs.mID), mRefs(0) {
}

SharedObject& SharedObject::operator=(const SharedObject &rhs) {
    FATAL_CHECK_EQ(mID, rhs.mID);
    return *this;
}

__END_NAMESPACE_ABE

#define BUFFER_START_MAGIC  0xbaaddead
#define BUFFER_END_MAGIC    0xdeadbaad
#define BUFFER_OBJECT_ID    FOURCC('sbuf')
__BEGIN_NAMESPACE_ABE

SharedBuffer::SharedBuffer() : SharedObject(BUFFER_OBJECT_ID),
    mAllocator(Nil), mData(Nil), mSize(0) { }

    SharedBuffer * SharedBuffer::allocate(const sp<Allocator> & _allocator, UInt32 sz) {
        // FIXME: if allocator is aligned, make sure data is also aligned
        const UInt32 allocLength = sizeof(SharedBuffer) + sz + sizeof(UInt32) * 2;

        // keep a strong ref local
        sp<Allocator> allocator = _allocator;
        SharedBuffer * shared = (SharedBuffer *)allocator->allocate(allocLength);
        memset((void*)shared, 0, sizeof(SharedBuffer));
        new (shared) SharedBuffer();

        shared->mAllocator  = allocator;
        shared->mSize       = sz;

        // put magic guard before and after data
        Char * data = (Char *)&shared[1];
        *(UInt32 *)data           = BUFFER_START_MAGIC;
        data                        += sizeof(UInt32);
        *(UInt32 *)(data + sz)    = BUFFER_END_MAGIC;
        shared->mData               = data;

        shared->RetainObject();
        return shared;
    }

void SharedBuffer::deallocate() {
    FATAL_CHECK_EQ(GetObjectID(), BUFFER_OBJECT_ID);
    FATAL_CHECK_EQ(((UInt32 *)mData)[-1], BUFFER_START_MAGIC);
    FATAL_CHECK_EQ(*(UInt32 *)(mData + mSize), BUFFER_END_MAGIC);

    // keep a strong ref local
    sp<Allocator> allocator = mAllocator;
    this->~SharedBuffer();
    allocator->deallocate(this);
}

UInt32 SharedBuffer::ReleaseBuffer(Bool keep) {
    FATAL_CHECK_EQ(GetObjectID(), BUFFER_OBJECT_ID);
    FATAL_CHECK_EQ(((UInt32 *)mData)[-1], BUFFER_START_MAGIC);
    FATAL_CHECK_EQ(*(UInt32 *)(mData + mSize), BUFFER_END_MAGIC);

    UInt32 refs = SharedObject::ReleaseObject(True);
    if (refs == 0 && !keep) {
        deallocate();
    }
    return refs;
}

SharedBuffer * SharedBuffer::edit() {
    FATAL_CHECK_EQ(GetObjectID(), BUFFER_OBJECT_ID);
    FATAL_CHECK_EQ(((UInt32 *)mData)[-1], BUFFER_START_MAGIC);
    FATAL_CHECK_EQ(*(UInt32 *)(mData + mSize), BUFFER_END_MAGIC);
    if (IsBufferNotShared()) return this;

    SharedBuffer * copy = SharedBuffer::allocate(mAllocator, mSize);
    memcpy(copy->mData, mData, mSize);

    ReleaseBuffer();
    return copy;
}

SharedBuffer * SharedBuffer::edit(UInt32 sz) {
    FATAL_CHECK_EQ(GetObjectID(), BUFFER_OBJECT_ID);
    FATAL_CHECK_EQ(((UInt32 *)mData)[-1], BUFFER_START_MAGIC);
    FATAL_CHECK_EQ(*(UInt32 *)(mData + mSize), BUFFER_END_MAGIC);

    if (IsBufferNotShared() && sz <= mSize) return this;

    if (IsBufferNotShared()) {
        // reallocate
        // keep a strong ref local
        sp<Allocator> allocator = mAllocator;
        const UInt32 allocLength = sizeof(SharedBuffer) + sz + 2 * sizeof(UInt32);
        SharedBuffer * shared = (SharedBuffer *)allocator->reallocate(this, allocLength);
        // fix context
        shared->mAllocator  = allocator;
        shared->mSize       = sz;

        Char * data                 = (Char *)&shared[1];
        *(UInt32 *)data           = BUFFER_START_MAGIC;
        data                        += sizeof(UInt32);
        *(UInt32 *)(data + sz)    = BUFFER_END_MAGIC;
        shared->mData               = data;
        return shared;
    }

    SharedBuffer * copy = SharedBuffer::allocate(mAllocator, sz);
    memcpy(copy->mData, mData, MIN(sz, mSize));

    ReleaseBuffer();
    return copy;
}

__END_NAMESPACE_ABE
