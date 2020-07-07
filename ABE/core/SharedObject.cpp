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

__BEGIN_NAMESPACE_ABE
#define INITIAL_VALUE   (0x10000)
SharedObject::Refs::Refs(SharedObject * p) : mObject(p), mRefs(INITIAL_VALUE), mWeakRefs(0) {
}

SharedObject::Refs::~Refs() {
}

UInt32 SharedObject::Refs::incRefs() {
    // if someone acquire weak refs before incRefs and
    // JUST release it before we are able to acquire
    // strong ref, it will be a problem. this happens
    // when acquiring weak refs to self in constructor.
    mRefs.cas(INITIAL_VALUE, 0);
    UInt32 refs = ++mRefs;
    if (refs == 1) {
        mObject->onFirstRetain();
    }
    ++mWeakRefs;
    return refs;
}

UInt32 SharedObject::Refs::decRefs(Bool keep) {
    // sanity check
    if (mRefs.load() == 0) {
        // do we need to assert here?
        ERROR("access object %p after release", mObject);
        return 0;
    }
    UInt32 refs = --mRefs;
    // delete object when strong ref all gone.
    if (refs == 0) {
        mObject->onLastRetain();
        if (!keep) {
            delete mObject;
        }
    }
    // no others acquired weak refs, release self.
    if (--mWeakRefs == 0) {
        delete this;
    }
    return refs;
}

UInt32 SharedObject::Refs::refCount() const {
    UInt32 refs = mRefs.load();
    if (refs == INITIAL_VALUE) return 0;
    return refs;
}

UInt32 SharedObject::Refs::weakRefCount() const {
    return mWeakRefs.load();
}

UInt32 SharedObject::Refs::incWeakRefs() {
    UInt32 refs = ++mWeakRefs;
    return refs;
}

UInt32 SharedObject::Refs::decWeakRefs() {
    // if someone acquire weak refs before strong refs and release
    // it before others have a chance to acquire strong refs, it
    // will be a problem. this happens when acquiring weak refs
    // to self in constructor.
    UInt32 refs = --mWeakRefs;
    if (refs == 0 && mRefs.load() != INITIAL_VALUE) {
        delete this;
    }
    return refs;
}

Bool SharedObject::Refs::retain() {
    // easy test, no strong refs exists.
    if (mRefs.load() == 0) {
        return False;
    }
    // hard test:
    // the strong refs may gone after the first test, so
    // we have to test again after acquire the strong refs here.
    UInt32 refs = ++mRefs;
    if (refs == 1) {
        // strong refs gone when we are acquring.
        // drop the strong refs.
        --mRefs;
        return False;
    }
    ++mWeakRefs;
    return True;
}
__END_NAMESPACE_ABE

const UInt32 kBufferObjectID = FOURCC('sbuf');
const UInt32 kBufferMagicStart  = FOURCC('sbf0');
const UInt32 kBufferMagicEnd    = FOURCC('sbf1');
__BEGIN_NAMESPACE_ABE

SharedBuffer::SharedBuffer() : SharedObject(kBufferObjectID),
mAllocator(Nil), mData(Nil), mSize(0) {
}

SharedBuffer * SharedBuffer::allocate(const sp<Allocator> & _allocator, UInt32 sz) {
    // FIXME: if allocator is aligned, make sure data is also aligned
    const UInt32 allocLength = sizeof(SharedBuffer) + sz + sizeof(UInt32) * 2;
    
    // keep a strong ref local
    sp<Allocator> allocator = _allocator;
    SharedBuffer * shared = (SharedBuffer *)allocator->allocate(allocLength);
    //memset((void*)shared, 0, sizeof(SharedBuffer));
    new (shared) SharedBuffer();
    
    shared->mAllocator  = allocator;
    shared->mSize       = sz;
    
    // put magic guard before and after data
    Char * data = (Char *)&shared[1];
    *(UInt32 *)data           = kBufferMagicStart;
    data                        += sizeof(UInt32);
    *(UInt32 *)(data + sz)    = kBufferMagicEnd;
    shared->mData               = data;
    
    shared->RetainObject();
    return shared;
}

void SharedBuffer::deallocate() {
    FATAL_CHECK_EQ(GetObjectID(), kBufferObjectID);
    FATAL_CHECK_EQ(((UInt32 *)mData)[-1], kBufferMagicStart);
    FATAL_CHECK_EQ(*(UInt32 *)(mData + mSize), kBufferMagicEnd);

    // keep a strong ref local
    sp<Allocator> allocator = mAllocator;
    this->~SharedBuffer();
    allocator->deallocate(this);
}

UInt32 SharedBuffer::ReleaseBuffer(Bool keep) {
    FATAL_CHECK_EQ(GetObjectID(), kBufferObjectID);
    FATAL_CHECK_EQ(((UInt32 *)mData)[-1], kBufferMagicStart);
    FATAL_CHECK_EQ(*(UInt32 *)(mData + mSize), kBufferMagicEnd);

    UInt32 refs = SharedObject::ReleaseObject(True);
    if (refs == 0 && !keep) {
        deallocate();
    }
    return refs;
}

SharedBuffer * SharedBuffer::edit() {
    FATAL_CHECK_EQ(GetObjectID(), kBufferObjectID);
    FATAL_CHECK_EQ(((UInt32 *)mData)[-1], kBufferMagicStart);
    FATAL_CHECK_EQ(*(UInt32 *)(mData + mSize), kBufferMagicEnd);
    if (IsBufferNotShared()) return this;

    SharedBuffer * copy = SharedBuffer::allocate(mAllocator, mSize);
    memcpy(copy->mData, mData, mSize);

    ReleaseBuffer();
    return copy;
}

SharedBuffer * SharedBuffer::edit(UInt32 sz) {
    FATAL_CHECK_EQ(GetObjectID(), kBufferObjectID);
    FATAL_CHECK_EQ(((UInt32 *)mData)[-1], kBufferMagicStart);
    FATAL_CHECK_EQ(*(UInt32 *)(mData + mSize), kBufferMagicEnd);

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
        *(UInt32 *)data           = kBufferMagicStart;
        data                        += sizeof(UInt32);
        *(UInt32 *)(data + sz)    = kBufferMagicEnd;
        shared->mData               = data;
        return shared;
    }

    SharedBuffer * copy = SharedBuffer::allocate(mAllocator, sz);
    memcpy(copy->mData, mData, MIN(sz, mSize));

    ReleaseBuffer();
    return copy;
}

__END_NAMESPACE_ABE
