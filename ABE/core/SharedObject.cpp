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

SharedObject::SharedObject(const uint32_t id) : mID(id), mRefs(0) {
}

SharedObject *  SharedObject::RetainObject() {
    DEBUG("retain %" PRIu32, mID);
    size_t refs = ++mRefs;
    if (refs == 1) onFirstRetain();
    return this;
}

size_t SharedObject::ReleaseObject(bool keep) {
    DEBUG("release %" PRIu32, mID);
    size_t refs = --mRefs;
    if (refs == 0) {
        onLastRetain();
        if (!keep) delete this;
    }
    return (size_t)refs;
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

__BEGIN_NAMESPACE_ABE

SharedBuffer::SharedBuffer() : SharedObject(OBJECT_ID_SHAREDBUFFER),
    mAllocator(NULL), mData(NULL), mSize(0) { }

    SharedBuffer * SharedBuffer::Create(const Object<Allocator> & _allocator, size_t sz) {
        // FIXME: if allocator is aligned, make sure data is also aligned
        const size_t allocLength = sizeof(SharedBuffer) + sz + sizeof(uint32_t) * 2;

        // keep a strong ref local
        Object<Allocator> allocator = _allocator;
        SharedBuffer * shared = (SharedBuffer *)allocator->allocate(allocLength);
        memset((void*)shared, 0, sizeof(SharedBuffer));
        new (shared) SharedBuffer();

        shared->mAllocator  = allocator;
        shared->mSize       = sz;

        // put magic guard before and after data
        char * data = (char *)&shared[1];
        *(uint32_t *)data           = BUFFER_START_MAGIC;
        data                        += sizeof(uint32_t);
        *(uint32_t *)(data + sz)    = BUFFER_END_MAGIC;
        shared->mData               = data;

        shared->RetainObject();
        return shared;
    }

void SharedBuffer::deallocate() {
    FATAL_CHECK_EQ(GetObjectID(), OBJECT_ID_SHAREDBUFFER);
    FATAL_CHECK_EQ(((uint32_t *)mData)[-1], BUFFER_START_MAGIC);
    FATAL_CHECK_EQ(*(uint32_t *)(mData + mSize), BUFFER_END_MAGIC);

    // keep a strong ref local
    Object<Allocator> allocator = mAllocator;
    this->~SharedBuffer();
    allocator->deallocate(this);
}

size_t SharedBuffer::ReleaseBuffer(bool keep) {
    FATAL_CHECK_EQ(GetObjectID(), OBJECT_ID_SHAREDBUFFER);
    FATAL_CHECK_EQ(((uint32_t *)mData)[-1], BUFFER_START_MAGIC);
    FATAL_CHECK_EQ(*(uint32_t *)(mData + mSize), BUFFER_END_MAGIC);

    size_t refs = SharedObject::ReleaseObject(true);
    if (refs == 0 && !keep) {
        deallocate();
    }
    return refs;
}

SharedBuffer * SharedBuffer::edit() {
    FATAL_CHECK_EQ(GetObjectID(), OBJECT_ID_SHAREDBUFFER);
    FATAL_CHECK_EQ(((uint32_t *)mData)[-1], BUFFER_START_MAGIC);
    FATAL_CHECK_EQ(*(uint32_t *)(mData + mSize), BUFFER_END_MAGIC);
    if (IsBufferNotShared()) return this;

    SharedBuffer * copy = SharedBuffer::Create(mAllocator, mSize);
    memcpy(copy->mData, mData, mSize);

    ReleaseBuffer();
    return copy;
}

SharedBuffer * SharedBuffer::edit(size_t sz) {
    FATAL_CHECK_EQ(GetObjectID(), OBJECT_ID_SHAREDBUFFER);
    FATAL_CHECK_EQ(((uint32_t *)mData)[-1], BUFFER_START_MAGIC);
    FATAL_CHECK_EQ(*(uint32_t *)(mData + mSize), BUFFER_END_MAGIC);

    if (IsBufferNotShared() && sz <= mSize) return this;

    if (IsBufferNotShared()) {
        // reallocate
        // keep a strong ref local
        Object<Allocator> allocator = mAllocator;
        const size_t allocLength = sizeof(SharedBuffer) + sz + 2 * sizeof(uint32_t);
        SharedBuffer * shared = (SharedBuffer *)allocator->reallocate(this, allocLength);
        // fix context
        shared->mAllocator  = allocator;
        shared->mSize       = sz;

        char * data                 = (char *)&shared[1];
        *(uint32_t *)data           = BUFFER_START_MAGIC;
        data                        += sizeof(uint32_t);
        *(uint32_t *)(data + sz)    = BUFFER_END_MAGIC;
        shared->mData               = data;
        return shared;
    }

    SharedBuffer * copy = SharedBuffer::Create(mAllocator, sz);
    memcpy(copy->mData, mData, MIN(sz, mSize));

    ReleaseBuffer();
    return copy;
}

__END_NAMESPACE_ABE
