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

static const UInt32 kDefaultCapacity = 4;

Vector::Vector(const sp<Allocator>& allocator,
               UInt32 capacity, UInt32 dataLength,
               type_destruct_t dtor, type_move_t move) :
SharedObject(FOURCC('!vec')), mAllocator(allocator),
mDataLength(dataLength), mCapacity(capacity), mItemCount(0),
mDeletor(dtor), mMover(move) {
    CHECK_GT(capacity, 0);
    DEBUG("constructor");
    mItems = mAllocator->allocate(mCapacity * mDataLength);
    CHECK_NULL(mItems);
}

void Vector::onFirstRetain() {
    
}

void Vector::onLastRetain() {
    clear();
    mAllocator->deallocate(mItems);
}

void* Vector::access(UInt32 index) {
    CHECK_LT(index, mItemCount);
    return (Char *)mItems + index * mDataLength;
}

const void* Vector::access(UInt32 index) const {
    CHECK_LT(index, mItemCount);
    return (Char *)mItems + index * mDataLength;
}

void Vector::clear() {
    if (mItemCount) remove(0, mItemCount);
}

// shrink memory to fit size
void Vector::shrink() {
    if (mItemCount == capacity()) return;

    DEBUG("shrink %zu vs %zu", mItemCount, capacity());

    // FIXME: if has trivial move, shrink by realloc
    void * items = mAllocator->allocate(mItemCount);
    mMover(items, mItems, mItemCount);
    
    mAllocator->deallocate(mItems);
    mItems = items;
    mCapacity = mItemCount;
}

// grow storage without construct items
void * Vector::grow(UInt32 pos, UInt32 amount) {
    CHECK_LE(pos, mItemCount);
    const UInt32 count  = mItemCount + amount;
    const UInt32 move   = mItemCount - pos;
    
    if (count > mCapacity) {
        while (mCapacity < count) mCapacity *= 2;
        
        Char * items = (Char *)mAllocator->allocate(mDataLength * mCapacity);
        
        if (pos > 0) {
            mMover(items, mItems, pos);
        }
        if (move) {
            mMover(items + (pos + amount) * mDataLength,
                   (Char *)mItems + pos * mDataLength,
                   move);
        }
        
        mAllocator->deallocate(mItems);
        mItems = items;
    } else if (move) {
        // move item@pos -> pos+amount
        Char * where = (Char *)mItems + pos * mDataLength;
        mMover(where + amount * mDataLength,
               where,
               move);
    }
    // caller MUST construct object in place
    mItemCount = count;
    return (Char *)mItems + pos * mDataLength;
}

// no auto memory shrink
void Vector::remove(UInt32 pos, UInt32 amount) {
    CHECK_GT(amount, 0);
    CHECK_LE(pos + amount, mItemCount);
    const UInt32 count  = mItemCount - amount;
    const UInt32 move   = mItemCount - (pos + amount);

    Char * where = (Char *)mItems + pos * mDataLength;

    // NO need to move item before pos
    // destruct at pos
    mDeletor(where, amount);

    // move
    if (move) {
        mMover(where, where + mDataLength * amount, move);
    }
    mItemCount = count;
}

void Vector::insert(UInt32 pos, const void * what, type_copy_t copy) {
    CHECK_LE(pos, mItemCount);

    void * where = grow(pos, 1);
    copy(where, what, 1);
}

void * Vector::emplace(UInt32 pos, type_construct_t ctor) {
    CHECK_LE(pos, mItemCount);

    void * where = grow(pos, 1);
    if (ctor) ctor(where, 1);

    return where;
}

void Vector::erase(UInt32 pos) {
    CHECK_LT(pos, mItemCount);
    remove(pos, 1);
}

void Vector::erase(UInt32 first, UInt32 last) {
    CHECK_LT(first, last);
    CHECK_LT(last, mItemCount);

    remove(first, last - first + 1);
}

// stable sort
void Vector::sort(type_compare_t cmp) {
    if (mItemCount <= 1) return;
    // NO _edit() here, as we will copy value later

    // moving value is very heavily
    // optimize: do NOT move value until sort done.
    UInt32 * index = (UInt32 *)mAllocator->allocate(mItemCount * sizeof(UInt32));
    for (UInt32 i = 0; i < mItemCount; ++i) index[i] = i;

    Char * item = (Char *)mItems;
#define ITEM(n)     (item + index[n] * mDataLength)

    Bool reorder = False;
#define SET_FLAG() do { if (ABE_UNLIKELY(reorder == False)) reorder = True; } while(0)

    UInt32 * index2 = (UInt32 *)mAllocator->allocate(mItemCount * sizeof(UInt32));
    for (UInt32 seg = 1; seg < mItemCount; seg += seg) {
        for (UInt32 i = 0; i < mItemCount; i += 2 * seg) {
            DEBUG("merge buck %u & %u", i, i + seg);
            UInt32 a        = i;
            UInt32 b        = MIN(a + seg, mItemCount);;
            const UInt32 e1 = b;
            const UInt32 e2 = MIN(b + seg, mItemCount);
            UInt32 k = a;
            CHECK_LT(a, e1);
            CHECK_LE(b, e2);

            // merge neighbor buck
            // optimize: take less cmp ops
            if (e1 > a && e2 > b && cmp(ITEM(e1 - 1), ITEM(b))) {   // a ... < b[0]
#if 1
                // optimize: using memcpy instead of loop
                memcpy(index2 + k, index + a, (e2 - a) * sizeof(UInt32));
                k += (e2 - a);
#else
                while (a < e1)  index2[k++] = index[a++];
                while (b < e2)  index2[k++] = index[b++];
#endif
            } else if (e2 > b && cmp(ITEM(e2 - 1), ITEM(a))) {  // b ... < a[0]
#if 1
                memcpy(index2 + k, index + b, (e2 - b) * sizeof(UInt32));
                k += (e2 - b);
                memcpy(index2 + k, index + a, (e1 - a) * sizeof(UInt32));
                k += (e1 - a);
#else
                while (b < e2)  index2[k++] = index[b++];
                while (a < e1)  index2[k++] = index[a++];
#endif
                SET_FLAG();
            } else {
                // normal procedure: take more cmp ops
                while (a < e1 && b < e2) {
                    if (cmp(ITEM(b), ITEM(a))) {    // b < a
                        index2[k++] = index[b++];
                        SET_FLAG();
                    } else {
                        index2[k++] = index[a++];
                    }
                }
#if 1
                if (a < e1) {
                    memcpy(index2 + k, index + a, (e1 - a) * sizeof(UInt32));
                    k += e1 - a;
                }

                if (b < e2) {
                    memcpy(index2 + k, index + b, (e2 - b) * sizeof(UInt32));
                    k += e2 - b;
                }
#else
                while (a < e1)  index2[k++] = index[a++];
                while (b < e2)  index2[k++] = index[b++];
#endif
            }
        }

        UInt32 * temp   = index;
        index           = index2;
        index2          = temp;
    }
    mAllocator->deallocate(index2);

    // optimize for no reorder
    if (reorder == False) {
        DEBUG("no reorder");
    } else {
        Char * dest = (Char *)mAllocator->allocate(mCapacity * mDataLength);
        for (UInt32 i = 0; i < mItemCount; ++i) {
            DEBUG("%u => %u", index[i], i);
            mMover(dest + i * mDataLength, ITEM(i), 1);
        }
        mAllocator->deallocate(mItems);
        mItems = dest;
    }

#undef ITEM
    mAllocator->deallocate(index);
}

__END_NAMESPACE_ABE_PRIVATE
