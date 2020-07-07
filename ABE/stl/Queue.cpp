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


// File:    Queue.cpp
// Author:  mtdcy.chen
// Changes:
//          1. 20160829     initial version
//

#define LOG_TAG "Queue"
//#define LOG_NDEBUG 0
#include "core/Log.h"

#include "Queue.h"
#include <stdlib.h>

// single producer & single consumer
// https://github.com/cameron314/readerwriterqueue
// multi producer & multi consumer
// https://github.com/cameron314/concurrentqueue
// others:
// https://www.boost.org/doc/libs/1_63_0/doc/html/boost/lockfree/queue.html
// http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.53.8674&rep=rep1&type=pdf

__BEGIN_NAMESPACE_ABE_PRIVATE

struct LockFreeQueueImpl::NodeImpl {
    NodeImpl *  mNext;
    void *      mData;
};

LockFreeQueueImpl::LockFreeQueueImpl(const TypeHelper& helper) :
    mTypeHelper(helper), mHead(Nil), mTail(Nil), mLength(0) {
        // use a dummy node
        // to avoid modify both mHead and mTail at push() or pop()
        mHead = mTail = allocateNode();
        mHead->mData = Nil;
    }

LockFreeQueueImpl::~LockFreeQueueImpl() {
    clear();
    freeNode((NodeImpl *)mHead);    // free dummy node
    mHead = mTail = Nil;
}

void LockFreeQueueImpl::clear() {
    volatile NodeImpl *head = ABE_ATOMIC_LOAD(&mHead);
    while (ABE_ATOMIC_LOAD(&mLength)) {
        if (ABE_ATOMIC_CAS(&mHead, &head, mHead->mNext)) {
            ABE_ATOMIC_SUB(&mLength, 1);

            atomic_fence();
            mTypeHelper.do_destruct(head->mNext->mData, 1);
            freeNode((NodeImpl *)head);
        }
    }
}

UInt32 LockFreeQueueImpl::size() const {
    return ABE_ATOMIC_LOAD(&mLength);
}

LockFreeQueueImpl::NodeImpl * LockFreeQueueImpl::allocateNode() {
    const UInt32 length = sizeof(NodeImpl) + mTypeHelper.size();
    NodeImpl * node = static_cast<NodeImpl*>(malloc(length));
    node->mNext = Nil;
    node->mData = node + 1;
    return node;
}

void LockFreeQueueImpl::freeNode(NodeImpl * node) {
    free(node);
}

// node = allocateNode();
// do_copy();
// mTail->mNext = node;
// mTail = node;
UInt32 LockFreeQueueImpl::push1(const void * what) {
    NodeImpl *node = allocateNode();
    mTypeHelper.do_copy(node->mData, what, 1);

    atomic_fence();
    ABE_ATOMIC_STORE(&mTail->mNext, node);
    ABE_ATOMIC_STORE(&mTail, node);
    return ABE_ATOMIC_ADD(&mLength, 1);
}

UInt32 LockFreeQueueImpl::pushN(const void * what) {
    DEBUG("%p: push %p", this, mTail->mData);
    NodeImpl *node = allocateNode();
    mTypeHelper.do_copy(node->mData, what, 1);

    atomic_fence();
    volatile NodeImpl *tail = ABE_ATOMIC_LOAD(&mTail);  // old tail
    // mTail = node;
    do {
        if (ABE_ATOMIC_CAS(&mTail, &tail, node)) {
            // fix next: tail->mNext = node
            ABE_ATOMIC_STORE(&tail->mNext, node);
            return ABE_ATOMIC_ADD(&mLength, 1);
        }
    } while (1);
    return ABE_ATOMIC_LOAD(&mLength);
}

// node = mHead;
// mHead = mHead->mNext;
// do_destruct
// freeNode();
Bool LockFreeQueueImpl::pop1(void * where) {
    if (ABE_ATOMIC_LOAD(&mLength)) {
        volatile NodeImpl * head = ABE_ATOMIC_LOAD(&mHead);
        ABE_ATOMIC_STORE(&mHead, mHead->mNext);
        ABE_ATOMIC_SUB(&mLength, 1);

        atomic_fence();
        if (where) {
            mTypeHelper.do_move(where, head->mNext->mData, 1);
        } else {
            mTypeHelper.do_destruct(head->mNext->mData, 1);
        }
        head->mNext->mData = Nil;
        freeNode((NodeImpl *)head);
        return True;
    }
    return False;
}

Bool LockFreeQueueImpl::popN(void * where) {
    Bool success = False;
    volatile NodeImpl *head = ABE_ATOMIC_LOAD(&mHead);
    while (ABE_ATOMIC_LOAD(&mLength)) {
        // mHead = mHead->mNext
        if (ABE_ATOMIC_CAS(&mHead, &head, mHead->mNext)) {
            ABE_ATOMIC_SUB(&mLength, 1);
            success = True;
            break;
        }
    }

    atomic_fence();
    if (success) {
        if (where) {
            mTypeHelper.do_move(where, head->mNext->mData, 1);
        } else {
            mTypeHelper.do_destruct(head->mNext->mData, 1);
        }
        head->mNext->mData = Nil;
        freeNode((NodeImpl *)head);
        return True;
    }
    return False;
}

__END_NAMESPACE_ABE_PRIVATE
