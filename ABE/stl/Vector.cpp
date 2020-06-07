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


// File:    Vector.cpp
// Author:  mtdcy.chen
// Changes:
//          1. 20160829     initial version
//          2. 20160920     add cow and copy support
//


#define LOG_TAG "Vector"
//#define LOG_NDEBUG 0
#include "ABE/core/Log.h"

#include "Vector.h"

#include <string.h>
#include <stdlib.h>

#define MIN(x, y)   ((x) < (y) ? (x) : (y))

__BEGIN_NAMESPACE_ABE_PRIVATE

static const size_t kDefaultCapacity = 4;

VectorImpl::VectorImpl(const sp<Allocator>& allocator,
        size_t capacity,
        const TypeHelper& helper) :
    mTypeHelper(helper),
    mAllocator(allocator), mStorage(NULL),
    mCapacity(capacity), mItemCount(0)
{
    CHECK_GT(capacity, 0);
    DEBUG("constructor 1");
    mStorage = SharedBuffer::Create(mAllocator, mTypeHelper.size() * mCapacity);
}

VectorImpl::VectorImpl(const VectorImpl& rhs) :
    mTypeHelper(rhs.mTypeHelper),
    mAllocator(rhs.mAllocator), mStorage(rhs.mStorage->RetainBuffer()),
    mCapacity(rhs.mCapacity), mItemCount(rhs.mItemCount)
{
    DEBUG("copy constructor");
}

VectorImpl::~VectorImpl() {
    DEBUG("destructor");
    _release(mStorage, mItemCount);
    mStorage = NULL;
}

VectorImpl& VectorImpl::operator=(const VectorImpl& rhs) {
    DEBUG("copy assignment");
    _release(mStorage, mItemCount);

    mTypeHelper = rhs.mTypeHelper;
    mAllocator  = rhs.mAllocator;
    mStorage    = rhs.mStorage->RetainBuffer();
    mCapacity   = rhs.mCapacity;
    mItemCount  = rhs.mItemCount;
    return *this;
}

void* VectorImpl::access(size_t index) {
    CHECK_LT(index, mItemCount);
    _edit();
    return (void *)(mStorage->data() + index * mTypeHelper.size());
}

const void* VectorImpl::access(size_t index) const {
    CHECK_LT(index, mItemCount);
    return (void *)(mStorage->data() + index * mTypeHelper.size());
}

void VectorImpl::clear() {
    if (mItemCount) _remove(0, mItemCount);
}

// shrink memory to fit size
void VectorImpl::shrink() {
    if (mItemCount == capacity()) return;

    DEBUG("shrink %zu vs %zu", mItemCount, capacity());

    // FIXME: if has trivial move, shrink by realloc
    if (0 && mStorage->IsBufferNotShared()) {
        DEBUG("shrink with realloc.");
        mStorage = mStorage->edit(mItemCount * mTypeHelper.size());
    } else {
        DEBUG("shrink heavily");
        SharedBuffer *old = mStorage;
        mStorage = SharedBuffer::Create(mAllocator, mItemCount * mTypeHelper.size());

        if (old->IsBufferNotShared()) {
            mTypeHelper.do_move(mStorage->data(),
                    old->data(),
                    mItemCount);
            _release(old, mItemCount, true);
        } else {
            mTypeHelper.do_copy(mStorage->data(),
                    old->data(),
                    mItemCount);
            _release(old, mItemCount, false);
        }
    }
}

// grow storage without construct items
char * VectorImpl::_grow(size_t pos, size_t amount) {
    CHECK_LE(pos, mItemCount);
    const size_t count  = mItemCount + amount;
    const size_t move   = mItemCount - pos;

    // not shared, and no grow
    if (mStorage->IsBufferNotShared() && count <= capacity()) {
        char * where = mStorage->data() + pos * mTypeHelper.size();
        if (move) {
            mTypeHelper.do_move(where + amount * mTypeHelper.size(),
                    where,
                    move);
        }
        // caller will construct object in place
        mItemCount = count;
        return where;
    }

    // grow by double capacity ?
    while (mCapacity < count)   mCapacity *= 2;

    // optimize for push() with grow
    // FIXME:
    if (0 && !move && /*has_trivial_move() &&*/ mStorage->IsBufferNotShared()) {
        // just resize
        DEBUG("grow by realloc");
        mStorage = mStorage->edit(mCapacity * mTypeHelper.size());
    } else {
        DEBUG("grow heavily");
        SharedBuffer * old = mStorage;
        mStorage = SharedBuffer::Create(mAllocator, mCapacity * mTypeHelper.size());

        // copy item before pos
        if (pos) {
            mTypeHelper.do_copy(mStorage->data(),
                    old->data(),
                    pos);
        }

        // copy item after pos
        if (move) {
            mTypeHelper.do_copy(mStorage->data() + (pos + amount) * mTypeHelper.size(),
                    old->data() + pos * mTypeHelper.size(),
                    move);
        }
        _release(old, mItemCount);
    }
    mItemCount  = count;
    return mStorage->data() + pos * mTypeHelper.size();
}

// no auto memory shrink
void VectorImpl::_remove(size_t pos, size_t amount) {
    CHECK_LE(pos + amount, mItemCount);
    const size_t count  = mItemCount - amount;
    const size_t move   = mItemCount - (pos + amount);

    if (mStorage->IsBufferNotShared()) {
        char * storage = mStorage->data() + mTypeHelper.size() * pos;

        // NO need to move item before pos

        // destruct at pos
        mTypeHelper.do_destruct(storage, amount);

        // move
        if (move) {
            mTypeHelper.do_move(storage, storage + mTypeHelper.size() * amount, move);
        }
    } else {
        SharedBuffer * old = mStorage;
        mStorage = SharedBuffer::Create(mAllocator, mCapacity * mTypeHelper.size());

        // copy item before pos
        mTypeHelper.do_copy(mStorage->data(),
                old->data(),
                pos);

        // no need to destruct at pos

        // copy
        if (move) {
            mTypeHelper.do_copy(mStorage->data() + mTypeHelper.size() * pos,
                    old->data() + mTypeHelper.size() * (pos + amount),
                    move);
        }

        _release(old, mItemCount);
    }
    mItemCount = count;
}

void VectorImpl::insert(size_t pos, const void* what) {
    CHECK_LE(pos, mItemCount);

    char * where = _grow(pos, 1);
    mTypeHelper.do_copy(where, what, 1);
}

void * VectorImpl::emplace(size_t pos, type_construct_t ctor) {
    CHECK_LE(pos, mItemCount);

    char * where = _grow(pos, 1);
    if (ctor) ctor(where, 1);

    return where;
}

void VectorImpl::erase(size_t pos) {
    CHECK_LT(pos, mItemCount);
    _remove(pos, 1);
}

void VectorImpl::erase(size_t first, size_t last) {
    CHECK_LT(first, last);
    CHECK_LT(last, mItemCount);

    _remove(first, last - first + 1);
}

void VectorImpl::_release(SharedBuffer *sb, size_t count, bool moved) {
    if (sb->ReleaseBuffer(true) == 0) {
        if (count && !moved) {
            mTypeHelper.do_destruct(sb->data(), count);
        }
        sb->deallocate();
    }
}

void VectorImpl::_edit() {
    if (mStorage->IsBufferNotShared()) return;

    DEBUG("edit heavily");
    SharedBuffer* old = mStorage;
    mStorage = SharedBuffer::Create(mAllocator, capacity() * mTypeHelper.size());

    mTypeHelper.do_copy(mStorage->data(),
            old->data(),
            mItemCount);

    _release(old, mItemCount);
}

// stable sort
void VectorImpl::sort(type_compare_t cmp) {
    if (mItemCount <= 1) return;
    // NO _edit() here, as we will copy value later

    // moving value is very heavily
    // optimize: do NOT move value until sort done.
    size_t * index = (size_t *)mAllocator->allocate(mItemCount * sizeof(size_t));
    for (size_t i = 0; i < mItemCount; ++i) index[i] = i;

    char * item = mStorage->data();
#define ITEM(n)     (item + index[n] * mTypeHelper.size())

    bool reorder = false;
#define SET_FLAG(x) do { if (__builtin_expect(x == false, false)) x = true; } while(0)

    size_t * index2 = (size_t *)mAllocator->allocate(mItemCount * sizeof(size_t));
    for (size_t seg = 1; seg < mItemCount; seg += seg) {
        for (size_t i = 0; i < mItemCount; i += 2 * seg) {
            DEBUG("merge buck %zu & %zu", i, i + seg);
            size_t a        = i;
            size_t b        = MIN(a + seg, mItemCount);;
            const size_t e1 = b;
            const size_t e2 = MIN(b + seg, mItemCount);
            size_t k = a;
            CHECK_LT(a, e1);
            CHECK_LE(b, e2);

            // merge neighbor buck
            // optimize: take less cmp ops
            if (e1 > a && e2 > b && cmp(ITEM(e1 - 1), ITEM(b))) {   // a ... < b[0]
#if 1
                // optimize: using memcpy instead of loop
                memcpy(index2 + k, index + a, (e2 - a) * sizeof(size_t));
                k += (e2 - a);
#else
                while (a < e1)  index2[k++] = index[a++];
                while (b < e2)  index2[k++] = index[b++];
#endif
            } else if (e2 > b && cmp(ITEM(e2 - 1), ITEM(a))) {  // b ... < a[0]
#if 1
                memcpy(index2 + k, index + b, (e2 - b) * sizeof(size_t));
                k += (e2 - b);
                memcpy(index2 + k, index + a, (e1 - a) * sizeof(size_t));
                k += (e1 - a);
#else
                while (b < e2)  index2[k++] = index[b++];
                while (a < e1)  index2[k++] = index[a++];
#endif
                SET_FLAG(reorder);
            } else {
                // normal procedure: take more cmp ops
                while (a < e1 && b < e2) {
                    if (cmp(ITEM(b), ITEM(a))) {    // b < a
                        index2[k++] = index[b++];
                        SET_FLAG(reorder);
                    } else {
                        index2[k++] = index[a++];
                    }
                }
#if 1
                if (a < e1) {
                    memcpy(index2 + k, index + a, (e1 - a) * sizeof(size_t));
                    k += e1 - a;
                }

                if (b < e2) {
                    memcpy(index2 + k, index + b, (e2 - b) * sizeof(size_t));
                    k += e2 - b;
                }
#else
                while (a < e1)  index2[k++] = index[a++];
                while (b < e2)  index2[k++] = index[b++];
#endif
            }
        }

        size_t * temp   = index;
        index           = index2;
        index2          = temp;
    }
    mAllocator->deallocate(index2);

    // optimize for no reorder
    if (reorder == false) {
        DEBUG("no reorder");
        if (mStorage->IsBufferNotShared()) {
            // NOTHING
        } else {
            SharedBuffer * old = mStorage;
            mStorage = SharedBuffer::Create(mAllocator, mTypeHelper.size() * capacity());

            mTypeHelper.do_copy(mStorage->data(),
                    old->data(),
                    mItemCount);

            _release(old, mItemCount);
        }
    } else {
        SharedBuffer * old = mStorage;
        mStorage = SharedBuffer::Create(mAllocator, mTypeHelper.size() * capacity());

        char * dest = mStorage->data();
        for (size_t i = 0; i < mItemCount; ++i) {
            DEBUG("%zu => %zu", index[i], i);
            mTypeHelper.do_copy(dest, ITEM(i), 1);
            dest += mTypeHelper.size();
        }
        _release(old, mItemCount);
    }

#undef ITEM
    mAllocator->deallocate(index);
}

__END_NAMESPACE_ABE_PRIVATE
