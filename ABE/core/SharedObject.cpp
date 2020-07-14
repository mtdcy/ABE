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
        FATAL(__FILE__ ":" __LITERAL_TO_STRING(__LINE__)              \
                " CHECK(" #a " " #op " " #b ") failed. ");          \
    }                                                               \
} while(0)

#define FATAL_CHECK_EQ(a, b)    FATAL_CHECK(a, b, ==)
#define FATAL_CHECK_GE(a, b)    FATAL_CHECK(a, b, >=)
#define FATAL_CHECK_GT(a, b)    FATAL_CHECK(a, b, >)

__BEGIN_NAMESPACE_ABE
// about SharedObject:
// 1. sp<T> tracks SharedObject and Refs.
// 2. wp<T> only tracks Refs.
//
// Cases:
// case 1: only strong refs
//      strong refs MUST delete object & Refs.
// case 2: both strong refs & weak refs
//      strong refs only delete object.
//      weak refs only delete Refs.
// case 3: only weak refs
//      weak refs MUST delete object & Refs.
// case 4: no strong or weak refs
//      object MUST delete self and Refs.
//
// Issues:
// 1. if someone acquire weak refs before incRefs and
//  JUST release it before we are able to acquire
//  strong ref, it will be a problem. this happens
//  when acquiring weak refs to self in constructor or
//  client do it by mistake.

// implement case 3/4 with an initial value
#define INITIAL_VALUE   UINT32_MAX
SharedObject::Refs::Refs(SharedObject * p) : mObject(p),
mRefs(INITIAL_VALUE), mWeakRefs(INITIAL_VALUE) { }

SharedObject::Refs::~Refs() {
}

UInt32 SharedObject::Refs::incRefs() {
    UInt32 refs = INITIAL_VALUE;
    if (mRefs.cas(refs, 1)) {
        mObject->onFirstRetain();
        incWeakRefs();
        return 1;
    } else {
        UInt32 refs = ++mRefs;
        if (refs == 1) {
            mObject->onFirstRetain();;
        }
        incWeakRefs();
        return refs;
    }
}

UInt32 SharedObject::Refs::decRefs() {
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
        delete mObject;
    }
    decWeakRefs();
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
    UInt32 refs = INITIAL_VALUE;
    if (mWeakRefs.cas(refs, 1)) {
        return 1;
    } else {
        UInt32 refs = ++mWeakRefs;
        return refs;
    }
}

UInt32 SharedObject::Refs::decWeakRefs() {
    UInt32 refs = --mWeakRefs;
    if (refs == 0) {
        // case 3
        if (mRefs.load() == INITIAL_VALUE) {
            delete mObject;
        }
        // case 2
        delete this;
    }
    return refs;
}

SharedObject * SharedObject::Refs::retain() {
    // easy case, all strong refs are gone.
    if (mRefs.load() == 0) {
        return Nil;
    }
    
    // hard case
    UInt32 refs = INITIAL_VALUE;
    if (mRefs.cas(refs, 1)) {
        // case 3
        // retain success
        mObject->onFirstRetain();
        incWeakRefs();
        return mObject;
    }
    
    // the strong refs may gone after the first test.
    // so acquire and test here again.
    refs = ++mRefs;
    if (refs == 1) {
        --mRefs;
        return Nil;
    }
    incWeakRefs();
    return mObject;
}

SharedObject::~SharedObject() {
    // case 4
    // object be deleted by client
    if (mRefs->mWeakRefs.load() == INITIAL_VALUE) {
        delete mRefs;
    }
}
__END_NAMESPACE_ABE

__BEGIN_NAMESPACE_ABE
const UInt32 kBufferMagicStart  = FOURCC('sbf0');
const UInt32 kBufferMagicEnd    = FOURCC('sbf1');

// no new or delete inside SharedBuffer, as new/delete
// will bypass Allocator.
SharedBuffer * SharedBuffer::Create(const sp<Allocator> & _allocator, UInt32 sz) {
    const UInt32 allocLength = sizeof(SharedBuffer) + sz + 2 * sizeof(UInt32);
    
    // keep a strong ref local
    sp<Allocator> allocator = _allocator;
    SharedBuffer * sb = (SharedBuffer *)allocator->allocate(allocLength);
    new (sb) SharedBuffer();
    
    sb->mAllocator          = allocator;
    sb->mSize               = sz;
    
    Char * data             = (Char *)&sb[1];
    // put magic guard before and after data
    *(UInt32 *)data         = kBufferMagicStart;
    data                    += sizeof(UInt32);
    *(UInt32 *)(data + sz)  = kBufferMagicEnd;
    sb->mData               = data;
    
    ++sb->mRefs;
    return sb;
}

SharedBuffer::~SharedBuffer() {
}

void SharedBuffer::DeleteBuffer() {
    FATAL_CHECK_EQ(((UInt32 *)mData)[-1], kBufferMagicStart);
    FATAL_CHECK_EQ(*(UInt32 *)(mData + mSize), kBufferMagicEnd);
    
    // keep a ref to mAllocator
    sp<Allocator> allocator = mAllocator;
    this->~SharedBuffer();
    allocator->deallocate(this);
}

SharedBuffer * SharedBuffer::RetainBuffer() {
    ++mRefs;
    return this;
}

UInt32 SharedBuffer::ReleaseBuffer(Bool keep) {
    FATAL_CHECK_EQ(((UInt32 *)mData)[-1], kBufferMagicStart);
    FATAL_CHECK_EQ(*(UInt32 *)(mData + mSize), kBufferMagicEnd);
    
    UInt32 refs = --mRefs;
    if (refs == 0 && !keep) {
        DeleteBuffer();
    }
    return refs;
}

SharedBuffer * SharedBuffer::edit(UInt32 sz) {
    FATAL_CHECK_EQ(((UInt32 *)mData)[-1], kBufferMagicStart);
    FATAL_CHECK_EQ(*(UInt32 *)(mData + mSize), kBufferMagicEnd);
    
    if (!sz) sz = mSize;
    if (IsBufferNotShared() && sz <= mSize) return this;
    
    // DON'T USE REALLOCATE HERE, AS WE DON'T KNOWN WHAT REALLOCATE
    // DO TO THE ORIGINAL BUFFER CONTENT, IF IT BREAK THE ORIGINAL
    // ONE, WE WILL UNABLE TO ReleaseBuffer()
    
    SharedBuffer * sb = SharedBuffer::Create(mAllocator, sz);
    memcpy(sb->mData, mData, MIN(sz, mSize));

    ReleaseBuffer();
    return sb;
}

__END_NAMESPACE_ABE
