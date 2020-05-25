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


// File:    Queue.h
// Author:  mtdcy.chen
// Changes:
//          1. 20160829     initial version
//
#ifndef ABE_HEADERS_STL_QUEUE_H
#define ABE_HEADERS_STL_QUEUE_H
#include <ABE/stl/TypeHelper.h>
__BEGIN_NAMESPACE_ABE_PRIVATE
/**
 * a lock free queue implement
 */
class ABE_EXPORT LockFreeQueueImpl {
    public:
        LockFreeQueueImpl(const TypeHelper& helper);
        ~LockFreeQueueImpl();

    protected:
        void            push1(const void * what);   // for single producer
        void            pushN(const void * what);   // for multi producer
        bool            pop1(void * what);          // for single consumer
        bool            popN(void * what);          // for multi consumer
        size_t          size() const;
        void            clear();

    private:
        struct NodeImpl;
        NodeImpl *      allocateNode();
        void            freeNode(NodeImpl *);

        TypeHelper          mTypeHelper;
        volatile NodeImpl * mHead;
        volatile NodeImpl * mTail;
        volatile size_t     mLength;

    private:
        DISALLOW_EVILS(LockFreeQueueImpl);
};
__END_NAMESPACE_ABE_PRIVATE

__BEGIN_NAMESPACE_ABE
namespace LockFree {
    template <class TYPE> class Queue : protected __NAMESPACE_ABE_PRIVATE::LockFreeQueueImpl, public NonSharedObject {
        public:
            ABE_INLINE Queue() : LockFreeQueueImpl(TypeHelperBuilder<TYPE, false, true, true>()) { }
            ABE_INLINE ~Queue() { }

            ABE_INLINE size_t      size() const        { return LockFreeQueueImpl::size();     }
            ABE_INLINE bool        empty() const       { return size() == 0;                   }
            ABE_INLINE void        clear()             { LockFreeQueueImpl::clear();           }
            ABE_INLINE void        push(const TYPE& v) { LockFreeQueueImpl::pushN(&v);         }
            ABE_INLINE bool        pop(TYPE& v)        { return LockFreeQueueImpl::popN(&v);   }
    };
};
__END_NAMESPACE_ABE
#endif // ABE_HEADERS_STL_QUEUE_H
