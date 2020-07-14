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


// File:    List.cpp
// Author:  mtdcy.chen
// Changes:
//          1. 20160829     initial version
//

#define LOG_TAG "List"
//#define LOG_NDEBUG 0
#include "ABE/core/Log.h"


#include "List.h"

#include <string.h>

#include <stdlib.h>

#define BUILD_LIST(x)   (x)->mNext = (x)->mPrev = (x)

__BEGIN_NAMESPACE_ABE_PRIVATE

void List::Node::unlink() {
    if (mNext) mNext->mPrev = mPrev;
    if (mPrev) mPrev->mNext = mNext;
    mNext = mPrev   = Nil;
}

List::Node * List::Node::link(Node * next) {
    CHECK_NULL(next);
    // XXX: next may be linked. leave it to client
    mNext           = next;
    next->mPrev     = this;
    return this;
}

List::Node * List::Node::insert(Node * next) {
    CHECK_NULL(next);
    mNext           = next;
    mPrev           = next->mPrev;
    mNext->mPrev    = this;
    if (mPrev) mPrev->mNext = this;
    return this;
}

List::Node * List::Node::append(Node * prev) {
    CHECK_NULL(prev);
    mPrev           = prev;
    mNext           = prev->mNext;
    mPrev->mNext    = this;
    if (mNext) mNext->mPrev = this;
    return this;
}

List::List(const sp<Allocator>& allocator,
           UInt32 size,
           type_destruct_t dtor) :
SharedObject(FOURCC('!lst')), mAllocator(allocator),
mDeletor(dtor), mDataLength(size), mListLength(0) {
    DEBUG("constructor");
    // empty list
    mHead.mNext = mHead.mPrev = &mHead;
}

void List::onFirstRetain() {
    DEBUG("onFirstRetain");
}

void List::onLastRetain() {
    DEBUG("onLastRetain");
    clear();
    CHECK_EQ(mListLength, 0);
}

List::Node* List::allocateNode() {
    const UInt32 headLength     = (sizeof(Node) + 15) & ~15;
    const UInt32 nodeLength    = headLength + mDataLength;

    Node * node  = static_cast<Node *>(mAllocator->allocate(nodeLength));
    CHECK_NULL(node);
    //DEBUG("allocate new list node %p", node);
    new (node) Node();
    node->mData = &node[1];
    return node;
}

void List::freeNode(Node * node) {
    DEBUG("free list node %p", node);
    mAllocator->deallocate(node);
}

void List::clear() {
    for (Node * node = mHead.mNext; node != &mHead;) {
        Node * next = node->mNext;
        node->unlink();
        mDeletor(node->mData, 1);
        freeNode(node);
        node = next;
    }
    CHECK_EQ(mHead.mNext, &mHead);
    CHECK_EQ(mHead.mPrev, &mHead);
    mListLength = 0;
}

List::Node& List::push(Node& pos, const void * what, type_copy_t copy) {
    Node * node = allocateNode();
    copy(node->mData, what, 1);
    node->insert(&pos);
    ++mListLength;
    return *node;
}

List::Node& List::emplace(Node &pos, type_construct_t ctor) {
    Node * node = allocateNode();
    if (ctor) ctor(node->mData, 1);
    node->insert(&pos);
    ++mListLength;
    return *node;
}

List::Node& List::pop(Node &pos) {
    Node * next = pos.next();
    pos.unlink();
    mDeletor(pos.mData, 1);
    freeNode(&pos);
    --mListLength;
    return *next;
}

// we are doing stable sort
void List::sort(type_compare_t cmp) {
    if (mListLength <= 1) return;

    if (mListLength == 2) {
        Node * first = mHead.mNext;
        Node * last  = mHead.mPrev;
        if (cmp(last->mData, first->mData)) {
            last->unlink();
            last->insert(first);
        }
        return;
    }

    const UInt32 buckLength = (mListLength + 1) / 2;
    // optimize: using buck to speed up
    //  TODO: optimize the size
    Node * buck = (Node*)mAllocator->allocate(sizeof(Node) * buckLength);
    Node * node = mHead.mNext;
    // step 1: merge neighbor node
    for (UInt32 i = 0; i < buckLength; ++i) {
        Node * next = node->mNext;

        BUILD_LIST(&buck[i]);

        node->unlink();
        node->append(&buck[i]);

        if (next != &mHead) {
            Node * _next = next->mNext;
            if (cmp(next->mData, node->mData)) {
                next->unlink();
                next->insert(node);
            } else {
                next->unlink();
                next->append(node);
            }
            next = _next;
        } else {
            CHECK_EQ(next, &mHead);
            break;
        }
        node = next;
    }

    // step 2: merge neighbor buck
    for (UInt32 seg = 1; seg < buckLength; seg += seg) {
        DEBUG("seg = %zu", seg);
        for (UInt32 i = 0; i < buckLength; i += 2 * seg) {
            if (i + seg >= buckLength) {
                // last buck
                break;
            }

            DEBUG("merge buck %zu & %zu", i, i + seg);
            Node * a = buck[i].mNext;
            Node * b = buck[i + seg].mNext;

            // merge buck[i+seg] to buck[i]
            while (a != &buck[i] && b != &buck[i + seg]) {
                if (cmp(b->mData, a->mData)) {    // b < a
                    Node * next = b->mNext;
                    b->unlink();
                    DEBUG("insert b before a");
                    // buck[i] ... <-> [b] <-> a
                    b->insert(a);
                    // move reference b
                    b = next;
                } else {
                    // move reference a
                    a = a->mNext;
                }
            }

            while (b != &buck[i + seg]) {
                DEBUG("insert b at buck[%zu] tail", i);
                // buck[i] ... <-> a <-> [b]
                Node * next = b->mNext;
                b->unlink();
                b->insert(a);
                // move reference b
                b = next;
            }
        }
    }

    mHead.mNext         = buck[0].mNext;
    mHead.mPrev         = buck[0].mPrev;
    mHead.mNext->mPrev  = &mHead;
    mHead.mPrev->mNext  = &mHead;
    mAllocator->deallocate(buck);
}

__END_NAMESPACE_ABE_PRIVATE
