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


// File:    List.cpp
// Author:  mtdcy.chen
// Changes:
//          1. 20160829     initial version
//

#define LOG_TAG "List"
//#define LOG_NDEBUG 0
#include "ABE/basic/Log.h"


#include "List.h"

#include <string.h>

#include <stdlib.h>

#define INSERT_SORT     0
#define BUILD_LIST(x)   (x)->mNext = (x)->mPrev = (x)

__BEGIN_NAMESPACE_ABE_PRIVATE

bool ListNodeImpl::orphan() const {
    return mNext == NULL && mPrev == NULL;
}

void ListNodeImpl::unlink() {
    if (mNext) mNext->mPrev = mPrev;
    if (mPrev) mPrev->mNext = mNext;
    mNext = mPrev   = NULL;
}

ListNodeImpl * ListNodeImpl::link(ListNodeImpl *next) {
    CHECK_NULL(next);
    // XXX: next may be linked. leave it to client
    mNext           = next;
    next->mPrev     = this;
    return this;
}

ListNodeImpl * ListNodeImpl::insert(ListNodeImpl * next) {
    CHECK_NULL(next);
    mNext           = next;
    mPrev           = next->mPrev;
    mNext->mPrev    = this;
    if (mPrev) mPrev->mNext = this;
    return this;
}

ListNodeImpl * ListNodeImpl::append(ListNodeImpl * prev) {
    CHECK_NULL(prev);
    mPrev           = prev;
    mNext           = prev->mNext;
    mPrev->mNext    = this;
    if (mNext) mNext->mPrev = this;
    return this;
}

ListImpl::ListImpl(const sp<Allocator>& allocator, const TypeHelper& helper) :
    mTypeHelper(helper),
    mAllocator(allocator), mStorage(NULL),
    mListLength(0)
{
    DEBUG("constructor");
    _prepare(mAllocator);
}

ListImpl::ListImpl(const ListImpl& rhs) :
    mTypeHelper(rhs.mTypeHelper),
    mAllocator(rhs.mAllocator), mStorage(rhs.mStorage->RetainBuffer()),
    mListLength(rhs.mListLength)
{
    DEBUG("copy constructor");
}

ListImpl::~ListImpl() {
    DEBUG("destructor");
    _clear();
    CHECK_EQ(mListLength, 0);
}

ListImpl& ListImpl::operator=(const ListImpl& rhs) {
    DEBUG("copy assignment");
    _clear();

    mTypeHelper     = rhs.mTypeHelper;
    mAllocator      = rhs.mAllocator;
    mStorage        = rhs.mStorage->RetainBuffer();
    mListLength     = rhs.mListLength;
    return *this;
}

ListNodeImpl* ListImpl::allocateNode() {
    const size_t headLength     = (sizeof(ListNodeImpl) + 15) & ~15;
    const size_t totalLength    = headLength + mTypeHelper.size();

    ListNodeImpl *node  = static_cast<ListNodeImpl*>(mAllocator->allocate(totalLength));
    CHECK_NULL(node);
    //DEBUG("allocate new list node %p", node);
    new (node) ListNodeImpl();
    node->mData     = (char*)node + headLength;
    return node;
}

void ListImpl::freeNode(ListNodeImpl* node) {
    DEBUG("free list node %p", node);
    mAllocator->deallocate(node);
}

void ListImpl::_prepare(const sp<Allocator>& allocator) {
    mStorage = SharedBuffer::Create(allocator, sizeof(ListNodeImpl));
    ListNodeImpl * head = (ListNodeImpl *)mStorage->data();
    new (head) ListNodeImpl;
    BUILD_LIST(head);
    mListLength = 0;
}

// cow support
ListNodeImpl* ListImpl::_edit() {
    if (mStorage->IsBufferShared()) {
        SharedBuffer * old = mStorage;
        ListNodeImpl * list0 = (ListNodeImpl *)old->data();

        // prepare new list context
        _prepare(mAllocator);
        ListNodeImpl * list = (ListNodeImpl *)mStorage->data();

        for (ListNodeImpl * node = list0->mNext; node != list0; node = node->mNext) {
            ListNodeImpl * temp = allocateNode();
            mTypeHelper.do_copy(temp->mData, node->mData, 1);
            temp->append(list->mPrev);
            ++mListLength;
        }

        // free old nodes when it is onlyOwnByUs
        if (old->ReleaseBuffer(true) == 0) {
            ListNodeImpl* node = list0->mNext;
            while (node != list0) {
                ListNodeImpl * next = node->mNext;
                node->unlink();
                mTypeHelper.do_destruct(node->mData, 1);
                freeNode(node);
                node = next;
            }
            old->deallocate();
        }
    }

    return (ListNodeImpl *)mStorage->data();
}

void ListImpl::_clear() {
    if (mStorage->ReleaseBuffer(true) == 0) {
        ListNodeImpl * list = (ListNodeImpl *)mStorage->data();
        ListNodeImpl * node = list->mNext;
        while (node != list) {
            ListNodeImpl * next = node->mNext;

            node->unlink();
            mTypeHelper.do_destruct(node->mData, 1);
            freeNode(node);

            node = next;
        }
        mStorage->deallocate();
        mStorage = NULL;
        mListLength = 0;
    }
}

void ListImpl::clear() {
    _clear();
    _prepare(mAllocator);
}

const ListNodeImpl& ListImpl::list() const {
    return *(const ListNodeImpl *)mStorage->data();
}

ListNodeImpl& ListImpl::list() {
    return *_edit();
}

ListNodeImpl& ListImpl::push(ListNodeImpl &pos, const void *what) {
    ListNodeImpl * node = allocateNode();
    mTypeHelper.do_copy(node->mData, what, 1);
    node->insert(&pos);
    ++mListLength;
    return *node;
}

ListNodeImpl& ListImpl::emplace(ListNodeImpl &pos, type_construct_t ctor) {
    ListNodeImpl * node = allocateNode();
    if (ctor) ctor(node->mData, 1);
    node->insert(&pos);
    ++mListLength;
    return *node;
}

ListNodeImpl& ListImpl::pop(ListNodeImpl &pos) {
    ListNodeImpl * next = pos.next();
    pos.unlink();
    mTypeHelper.do_destruct(pos.mData, 1);
    freeNode(&pos);
    --mListLength;
    return *next;
}

// we are doing stable sort
void ListImpl::sort(type_compare_t cmp) {
    if (mListLength <= 1) return;

    ListNodeImpl* list = _edit();

    if (mListLength == 2) {
        ListNodeImpl * first = list->mNext;
        ListNodeImpl * last  = list->mPrev;
        if (cmp(last->mData, first->mData)) {
            last->unlink();
            last->insert(first);
        }
        return;
    }

    const size_t buckLength = (mListLength + 1) / 2;
    // optimize: using buck to speed up
    //  TODO: optimize the size
    ListNodeImpl * buck = (ListNodeImpl*)mAllocator->allocate(sizeof(ListNodeImpl) * buckLength);
    ListNodeImpl * node = list->mNext;
    // step 1: merge neighbor node
    for (size_t i = 0; i < buckLength; ++i) {
        ListNodeImpl * next = node->mNext;

        BUILD_LIST(&buck[i]);

        node->unlink();
        node->append(&buck[i]);

        if (next != list) {
            ListNodeImpl * _next = next->mNext;
            if (cmp(next->mData, node->mData)) {
                next->unlink();
                next->insert(node);
            } else {
                next->unlink();
                next->append(node);
            }
            next = _next;
        } else {
            CHECK_EQ(next, list);
            break;
        }
        node = next;
    }

    // step 2: merge neighbor buck
    for (size_t seg = 1; seg < buckLength; seg += seg) {
        DEBUG("seg = %zu", seg);
        for (size_t i = 0; i < buckLength; i += 2 * seg) {
            if (i + seg >= buckLength) {
                // last buck
                break;
            }

            DEBUG("merge buck %zu & %zu", i, i + seg);
            ListNodeImpl * a = buck[i].mNext;
            ListNodeImpl * b = buck[i + seg].mNext;

            // merge buck[i+seg] to buck[i]
            while (a != &buck[i] && b != &buck[i + seg]) {
                if (cmp(b->mData, a->mData)) {    // b < a
                    ListNodeImpl * next = b->mNext;
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
                ListNodeImpl * next = b->mNext;
                b->unlink();
                b->insert(a);
                // move reference b
                b = next;
            }
        }
    }

    list->mNext         = buck[0].mNext;
    list->mPrev         = buck[0].mPrev;
    list->mNext->mPrev  = list;
    list->mPrev->mNext  = list;
    mAllocator->deallocate(buck);
}

__END_NAMESPACE_ABE_PRIVATE
