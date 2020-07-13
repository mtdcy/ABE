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

HashTable::HashTable(const sp<Allocator>& allocator,
                     UInt32 tableLength,
                     UInt32 keyLength,
                     UInt32 dataLength,
                     type_hash_t hash,
                     type_compare_t cmp,
                     type_copy_t copy,
                     type_destruct_t key_dtor,
                     type_destruct_t data_dtor) :
SharedObject(FOURCC('!tbl')), mAllocator(allocator), mKeyLength(keyLength), mDataLength(dataLength),
mTableLength(tableLength), mNumElements(0), mHash(hash), mKeyCompare(cmp), mKeyCopy(copy),
mKeyDeletor(key_dtor), mDataDeletor(data_dtor) {
    const UInt32 allocLength = sizeof(Node *) * mTableLength;
    mBuckets = (Node **)mAllocator->allocate(allocLength);
    memset(mBuckets, 0, allocLength);
}

void HashTable::onFirstRetain() {
    
}

void HashTable::onLastRetain() {
    clear();
    mAllocator->deallocate(mBuckets);
}

// clear only Elements, not the bucket storage
void HashTable::clear() {
    DEBUG("clear: size = %u", mNumElements);
    if (mNumElements == 0) return;
    
    for (UInt32 index = 0; index < mTableLength; ++index) {
        Node * node = mBuckets[index];
        while (node) {
            Node * next = node->mNext;
            DEBUG("- hash %u index %u", node->mHash, index);
            freeNode(node);
            node = next;
        }
    }
    mNumElements = 0;
}

HashTable::Node::Node(UInt32 hash) : mHash(hash), mKey(Nil), mValue(Nil), mNext(Nil) {
}

HashTable::Node::~Node() {
    mKey = mValue = mNext = Nil;
}

HashTable::Node * HashTable::allocateNode(UInt32 hash) {
    Node * e = (Node *)mAllocator->allocate(sizeof(Node) + mKeyLength + mDataLength);
    new (e) Node(hash);
    // do not construct data here
    e->mKey     = &e[1];
    e->mValue   = (Char *)e->mKey + mKeyLength;
    return e;
}

void HashTable::freeNode(Node *e) {
    mKeyDeletor(e->mKey, 1);
    mDataDeletor(e->mValue, 1);
    e->~Node();
    mAllocator->deallocate(e);
}

void HashTable::insert(const void * k, const void * v, type_copy_t copy) {
    const UInt32 hash = mHash(k);
    const UInt32 index = hash % mTableLength;
    Node * node  = allocateNode(hash);
    mKeyCopy(node->mKey, k, 1);
    copy(node->mValue, v, 1);
    
    DEBUG("+ hash %u index %u", hash, index);
    node->mNext = mBuckets[index];
    mBuckets[index] = node;
    
    // HANDLE COLLISION ?

    ++mNumElements;
    grow();
}

void HashTable::grow() {
    // FIXME: implement Incremental resizing
    //  be careful with the price enlarging the hash table all at once
    // load factor .75
    if (mNumElements > (mTableLength * 3) / 4) {
        DEBUG("table grow %u -> %u", mTableLength, mTableLength * 2);

        const UInt32 tableLength = mTableLength * 2;    // double table length
        const UInt32 allocLength = sizeof(Node *) * tableLength;
        Node ** buck = (Node **)mAllocator->allocate(allocLength);
        memset(buck, 0, allocLength);

        for (UInt32 i = 0; i < mTableLength; ++i) {
            Node * node = mBuckets[i];
            while (node) {
                Node * next = node->mNext;
                UInt32 index = node->mHash % tableLength;
                DEBUG("> hash %u index %zu", node->mHash, index);
                node->mNext = buck[index];
                buck[index] = node;
                node = next;
            }
        }
        // release old bucket, not Elements
        mAllocator->deallocate(mBuckets);
        mBuckets        = buck;
        mTableLength    = tableLength;
    }
}

UInt32 HashTable::erase(const void * k) {
    const UInt32 hash = mHash(k);
    const UInt32 index  = hash % mTableLength;
    Node ** p = &mBuckets[index];
    while (*p) {
        if (hash == (*p)->mHash && mKeyCompare(k, (*p)->mKey)) {
            Node * node = *p;
            *p = node->mNext;
            --mNumElements;

            DEBUG("- hash %u index %u", node->mHash, index);
            freeNode(node);
            shrink();
            return 1;
        }
        p = &((*p)->mNext);
    }
    return 0;
}

void HashTable::shrink() {
    // TODO: implement buckets shrink
}

void * HashTable::find(const void * k) {
    const UInt32 hash = mHash(k);
    const UInt32 index = hash % mTableLength;
    Node * node = mBuckets[index];
    while (node) {
        if (hash == node->mHash && mKeyCompare(k, node->mKey)) {
            return node->mValue;
        }
        node = node->mNext;
    }
    return Nil;
}

const void * HashTable::find(const void * k) const {
    const UInt32 hash = mHash(k);
    const UInt32 index = hash % mTableLength;
    Node * node = mBuckets[index];
    while (node) {
        if (hash == node->mHash && mKeyCompare(k, node->mKey)) {
            return node->mValue;
        }
        node = node->mNext;
    }
    return Nil;
}

void * HashTable::access(const void * k) {
    const UInt32 hash = mHash(k);
    const UInt32 index = hash % mTableLength;
    Node * node = mBuckets[index];
    while (node) {
        if (hash == node->mHash && mKeyCompare(k, node->mKey)) {
            return node->mValue;
        }
        node = node->mNext;
    }
    return Nil;
}

const void * HashTable::access(const void * k) const {
    const UInt32 hash = mHash(k);
    const UInt32 index = hash % mTableLength;
    Node * node = mBuckets[index];
    while (node) {
        if (hash == node->mHash && mKeyCompare(k, node->mKey)) {
            return node->mValue;
        }
        node = node->mNext;
    }
    return Nil;
}

void * HashTable::emplace(const void * k, type_construct_t ctor) {
    const UInt32 hash = mHash(k);
    const UInt32 index = hash % mTableLength;
    Node * node = mBuckets[index];
    while (node) {
        if (hash == node->mHash && mKeyCompare(k, node->mKey)) {
            return node->mValue;
        }
        node = node->mNext;
    }
    DEBUG("+ hash %u index %u", hash, index);
    node = allocateNode(hash);
    ctor(node->mValue, 1);
    node->mNext = mBuckets[index];
    mBuckets[index] = node;
    return node->mValue;
}

HashTable::Node * HashTable::next(const Node * node) const {
    // return first node
    if (node == Nil) {
        for (UInt32 i = 0; i < mTableLength; ++i) {
            Node * node = mBuckets[i];
            if (node) {
                return node;
            }
        }
    }
    if (node == Nil) return Nil;
    // return next node in the bucket
    if (node->mNext) return node->mNext;
    
    // return node in next bucket
    const UInt32 index = node->mHash % mTableLength;
    for (UInt32 i = index + 1; i < mTableLength; ++i) {
        Node * node = mBuckets[i];
        if (node) {
            return node;
        }
    }
    return Nil;
}

__END_NAMESPACE_ABE_PRIVATE
