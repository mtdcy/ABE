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


// File:    HashTable.cpp
// Author:  mtdcy.chen
// Changes:
//          1. 20181113     initial version
//


#define LOG_TAG "HashTable"
//#define LOG_NDEBUG 0
#include "ABE/core/Log.h"

#include "HashTable.h"

#define POW_2(x)    (1 << (32 - __builtin_clz((x)-1)))

#include <stdlib.h>
#include <string.h>

__BEGIN_NAMESPACE_ABE_PRIVATE

HashTableImpl::HashTableImpl(const sp<Allocator>& allocator,
        UInt32 tableLength,
        const TypeHelper& keyHelper,
        const TypeHelper& valueHelper,
        type_compare_t equal) :
    mKeyHelper(keyHelper), mValueHelper(valueHelper), mKeyCompare(equal),
    mAllocator(allocator), mStorage(Nil),
    mTableLength(POW_2(tableLength)), mNumElements(0)
{
    const UInt32 allocLength = sizeof(Element *) * mTableLength;
    mStorage = SharedBuffer::allocate(mAllocator, allocLength);
    memset(mStorage->data(), 0, allocLength);
}

HashTableImpl::HashTableImpl(const HashTableImpl& rhs) :
    mKeyHelper(rhs.mKeyHelper), mValueHelper(rhs.mValueHelper), mKeyCompare(rhs.mKeyCompare),
    mAllocator(rhs.mAllocator), mStorage(rhs.mStorage->RetainBuffer()),
    mTableLength(rhs.mTableLength), mNumElements(rhs.mNumElements) {

    }

HashTableImpl& HashTableImpl::operator=(const HashTableImpl & rhs) {
    // must be the same type
    _release(mStorage);
    // copy allocator & storage
    mKeyHelper      = rhs.mKeyHelper;
    mValueHelper    = rhs.mValueHelper;
    mKeyCompare     = rhs.mKeyCompare;
    mAllocator      = rhs.mAllocator;
    mStorage        = rhs.mStorage->RetainBuffer();
    mTableLength    = rhs.mTableLength;
    mNumElements    = rhs.mNumElements;

    return *this;
}

HashTableImpl::~HashTableImpl() {
    _release(mStorage);
    mNumElements = 0;
    mTableLength = 0;
}

// clear only Elements, not the bucket storage
void HashTableImpl::clear() {
    if (mNumElements == 0) return;
    Element ** buck = (Element **)mStorage->data();
    for (UInt32 index = 0; index < mTableLength; ++index) {
        Element * e = buck[index];
        while (e) {
            Element * next  = e->mNext;
            deallocateElement(e);
            e = next;
        }
        buck[index] = Nil;
    }
    mNumElements = 0;
}

// clear both Elements and bucket storage
void HashTableImpl::_release(SharedBuffer * storage) {
    if (storage->ReleaseBuffer(True) == 0) {
        Element ** buck = (Element **)storage->data();
        for (UInt32 index = 0; index < mTableLength; ++index) {
            Element * e = buck[index];
            while (e) {
                Element * next = e->mNext;
                deallocateElement(e);
                e = next;
            }
            buck[index] = Nil;
        }
        storage->deallocate();
    }
}

// 'cow': copy bucket and Elements on write
HashTableImpl::Element ** HashTableImpl::_edit() {
    if (mStorage->IsBufferShared()) {
        DEBUG("copy on write");
        SharedBuffer * old  = mStorage;
        Element ** buck0    = (Element **)old->data();

        const UInt32 allocLength = sizeof(Element *) * mTableLength;
        mStorage = SharedBuffer::allocate(mAllocator, allocLength);
        Element ** buck     = (Element **)mStorage->data();
        memset(buck, 0, allocLength);

        for (UInt32 index = 0; index < mTableLength; ++index) {
            Element * e0    = buck0[index];
            Element ** p    = &buck[index];
            while (e0) {
                Element * e = allocateElement(e0->mHash);
                mKeyHelper.do_copy(e->mKey, e0->mKey, 1);
                mValueHelper.do_copy(e->mValue, e0->mValue, 1);
                *p  = e;
                p   = &(e->mNext);
                e0  = e0->mNext;
            }
        }

        _release(old);
    }
    return (Element **)mStorage->data();
}

HashTableImpl::Element::Element(UInt32 hash) : mHash(hash), mKey(Nil), mValue(Nil), mNext(Nil) {
}

HashTableImpl::Element::~Element() {
    mKey = mValue = mNext = Nil;
}

HashTableImpl::Element * HashTableImpl::allocateElement(UInt32 hash) {
    Element * e = (Element *)mAllocator->allocate(sizeof(Element) + mKeyHelper.size() + mValueHelper.size());
    new (e) Element(hash);
    // do not construct data here
    e->mKey     = &e[1];
    e->mValue   = (Char *)e->mKey + mKeyHelper.size();
    return e;
}

void HashTableImpl::deallocateElement(Element *e) {
    mKeyHelper.do_destruct(e->mKey, 1);
    mValueHelper.do_destruct(e->mValue, 1);
    e->~Element();
    mAllocator->deallocate(e);
}

void HashTableImpl::insert(const void * k, const void * v, UInt32 hash) {
    const UInt32 index = hash % mTableLength;
    Element *e  = allocateElement(hash);
    mKeyHelper.do_copy(e->mKey, k, 1);
    mValueHelper.do_copy(e->mValue, v, 1);

    Element ** buck = _edit();
    Element ** p    = &buck[index];
    DEBUG("+ hash %zu index %zu", hash, index);
    while (*p) {
        DEBUG("collision");
        CHECK_FALSE(mKeyCompare(k, (*p)->mKey));
        p = &((*p)->mNext);
    }
    *p = e;
    ++mNumElements;

    grow();
}

void HashTableImpl::grow() {
    // FIXME: implement Incremental resizing
    //  be careful with the price enlarging the hash table all at once
    // load factor .75
    if (mNumElements > (mTableLength * 3) / 4) {
        DEBUG("table grow %zu -> %zu", mTableLength, mTableLength * 2);
        SharedBuffer * old = mStorage;
        Element ** buck0 = (Element **)old->data();

        const UInt32 tableLength = mTableLength * 2;    // Float64 table length
        const UInt32 allocLength = sizeof(Element *) * tableLength;
        mStorage = SharedBuffer::allocate(mAllocator, allocLength);
        Element ** buck = (Element **)mStorage->data();
        memset(buck, 0, allocLength);

        for (UInt32 i = 0; i < mTableLength; ++i) {
            Element * e     = buck0[i];
            while (e) {
                Element * next  = e->mNext;
                UInt32 index    = e->mHash % tableLength;
                DEBUG("> hash %zu index %zu", e->mHash, index);
                e->mNext        = buck[index];
                buck[index]     = e;
                e               = next;
            }
        }
        // release old bucket, not Elements
        old->ReleaseBuffer();
        mTableLength    = tableLength;
    }
}

UInt32 HashTableImpl::erase(const void * k, UInt32 hash) {
    const UInt32 index  = hash % mTableLength;
    Element ** buck = _edit();
    Element ** p    = &buck[index];
    while (*p) {
        if (hash == (*p)->mHash && mKeyCompare(k, (*p)->mKey)) {
            Element * e     = *p;
            *p              = e->mNext;
            --mNumElements;

            deallocateElement(e);
            shrink();
            return 1;
        }
        p   = &((*p)->mNext);
    }
    return 0;
}

void HashTableImpl::shrink() {
    // TODO: implement buckets shrink
}

void * HashTableImpl::find(const void * k, UInt32 hash) {
    const UInt32 index = hash % mTableLength;
    Element ** buck = _edit();
    Element * p     = buck[index];
    while (p) {
        if (hash == p->mHash && mKeyCompare(k, p->mKey)) {
            return p->mValue;
        }
        p = p->mNext;
    }
    return Nil;
}

const void * HashTableImpl::find(const void * k, UInt32 hash) const {
    const UInt32 index = hash % mTableLength;
    Element ** buck = (Element **)mStorage->data();
    Element * p     = buck[index];
    while (p) {
        if (hash == p->mHash && mKeyCompare(k, p->mKey)) {
            return p->mValue;
        }
        p = p->mNext;
    }
    return Nil;
}

void * HashTableImpl::access(const void * k, UInt32 hash) {
    const UInt32 index = hash % mTableLength;
    Element ** buck = _edit();
    Element * p     = buck[index];
    while (p) {
        if (hash == p->mHash && mKeyCompare(k, p->mKey)) {
            return p->mValue;
        }
        p = p->mNext;
    }
    CHECK_NULL(p);
    return Nil;
}

const void * HashTableImpl::access(const void * k, UInt32 hash) const {
    const UInt32 index = hash % mTableLength;
    Element ** buck = (Element **)mStorage->data();
    Element * p     = buck[index];
    while (p) {
        if (hash == p->mHash && mKeyCompare(k, p->mKey)) {
            return p->mValue;
        }
        p = p->mNext;
    }
    CHECK_NULL(p);
    return Nil;
}

HashTableImpl::Element * HashTableImpl::next(const Element *elem, UInt32 *index) {
    CHECK_NULL(index);
    // should not shared this hash table during iterator
    CHECK_TRUE(mStorage->IsBufferNotShared());
    Element ** buck = (Element **)mStorage->data();
    Element * _next = Nil;
    if (elem) {
        _next = elem->mNext;
        if (_next) return _next;
        *index += 1;
    }

    while (_next == Nil && *index < mTableLength) {
        _next = buck[*index];
        if (_next) break;
        *index += 1;
    }

    return _next;
}

const HashTableImpl::Element * HashTableImpl::next(const Element *elem, UInt32 *index) const {
    CHECK_NULL(index);
    // const_iterator is ok even hash table is shared
    Element ** buck = (Element **)mStorage->data();
    Element * _next = Nil;
    if (elem) {
        _next = elem->mNext;
        if (_next) return _next;
        *index += 1;
    }

    while (_next == Nil && *index < mTableLength) {
        _next = buck[*index];
        if (_next) break;
        *index += 1;
    }

    return _next;
}

__END_NAMESPACE_ABE_PRIVATE
