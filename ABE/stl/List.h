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


// File:    List.h
// Author:  mtdcy.chen
// Changes: 
//          1. 20160829     initial version
//

#ifndef _TOOLKIT_HEADERS_STL_LIST_H
#define _TOOLKIT_HEADERS_STL_LIST_H 

#include <ABE/stl/TypeHelper.h>
#include <ABE/basic/SharedBuffer.h>

////////////////////////////////////////////////////////////////////////////// 
// c++ implementation of doubly-linked list container. 
#ifdef __cplusplus

__BEGIN_NAMESPACE_ABE_PRIVATE

class ListImpl;
class __ABE_HIDDEN ListNodeImpl {
    protected:
        friend class ListImpl;
        void            *mData;
        ListNodeImpl    *mPrev;
        ListNodeImpl    *mNext;

    protected:
        ListNodeImpl() : mData(NULL), mPrev(NULL), mNext(NULL) { }

    public:
        __ABE_INLINE void *          data() const    { return mData; }
        __ABE_INLINE ListNodeImpl *  next() const    { return mNext; }
        __ABE_INLINE ListNodeImpl *  prev() const    { return mPrev; }

    protected:
        bool            orphan() const;
        void            unlink();
        ListNodeImpl *  link(ListNodeImpl * next);
        ListNodeImpl *  insert(ListNodeImpl * next);
        ListNodeImpl *  append(ListNodeImpl * prev);
};

class ListImpl {
    public:
        ListImpl(const sp<Allocator>& allocator, const TypeHelper& helper);

        ListImpl(const ListImpl& rhs);

        ~ListImpl();

        ListImpl& operator=(const ListImpl& rhs);

    protected:
        __ABE_INLINE size_t              size() const    { return mListLength; }
        void                clear();

    protected:
        const ListNodeImpl& list() const;
        ListNodeImpl&       list();
        ListNodeImpl&       push(ListNodeImpl& pos, const void * what);
        ListNodeImpl&       emplace(ListNodeImpl& pos, type_construct_t ctor);
        ListNodeImpl&       pop(ListNodeImpl& pos);

        void                sort(type_compare_t);

    private:
        ListNodeImpl*       allocateNode();
        void                freeNode(ListNodeImpl* node);

        void                _prepare(const sp<Allocator>& allocator);
        ListNodeImpl*       _edit();
        void                _clear();

    private:
        TypeHelper          mTypeHelper;
        sp<Allocator>       mAllocator;
        SharedBuffer *      mStorage;
        size_t              mListLength;
};
__END_NAMESPACE_ABE_PRIVATE

__BEGIN_NAMESPACE_ABE
template <typename TYPE> class List : private __NAMESPACE_ABE_PRIVATE::ListImpl {
    protected:
        // iterator for List, bidirection iterator
        template <typename NODE_TYPE, typename VALUE_TYPE> class Iterator {
            public:
                __ABE_INLINE Iterator() : mNode(NULL) { }
                __ABE_INLINE Iterator(NODE_TYPE node) : mNode(node) { }
                __ABE_INLINE ~Iterator() { }

            public:
                __ABE_INLINE Iterator&   operator++()    { mNode = mNode->next(); return *this;              }   // pre-increment
                __ABE_INLINE Iterator    operator++(int) { Iterator old(*this); operator++(); return old;    }   // post-increment
                __ABE_INLINE Iterator&   operator--()    { mNode = mNode->prev(); return *this;              }   // pre-decrement
                __ABE_INLINE Iterator    operator--(int) { Iterator old(*this); operator--(); return old;    }   // post-decrement

                __ABE_INLINE VALUE_TYPE& operator*()     { return *(static_cast<VALUE_TYPE*>(mNode->data()));}
                __ABE_INLINE VALUE_TYPE* operator->()    { return static_cast<VALUE_TYPE*>(mNode->data());   }

            public:
                __ABE_INLINE bool        operator==(const Iterator& rhs) const   { return mNode == rhs.mNode;    }
                __ABE_INLINE bool        operator!=(const Iterator& rhs) const   { return mNode != rhs.mNode;    }

            protected:
                friend class List<VALUE_TYPE>;
                NODE_TYPE       mNode;
        };

    public:
        typedef Iterator<__NAMESPACE_ABE_PRIVATE::ListNodeImpl *, TYPE> iterator;
        typedef Iterator<const __NAMESPACE_ABE_PRIVATE::ListNodeImpl *, const TYPE> const_iterator;

    public:
        __ABE_INLINE List(const sp<Allocator>& allocator = kAllocatorDefault) :
            ListImpl(allocator, TypeHelperBuilder<TYPE, false, true, false>()) { }

        __ABE_INLINE ~List() { }

    public:
        __ABE_INLINE void    clear()                     { return ListImpl::clear();         }
        __ABE_INLINE size_t  size() const                { return ListImpl::size();          }
        __ABE_INLINE bool    empty() const               { return size() == 0;               }

    public:
        // stable sort, uses operator< or custom compare
        typedef bool (*compare_t)(const TYPE*, const TYPE*);
        __ABE_INLINE void    sort(compare_t cmp)         { ListImpl::sort(reinterpret_cast<type_compare_t>(cmp));    }
        __ABE_INLINE void    sort()                      { ListImpl::sort(type_compare_less<TYPE>);                  }

    public:
        __ABE_INLINE TYPE&       front()                 { return *static_cast<TYPE*>(list().next()->data());        }
        __ABE_INLINE const TYPE& front() const           { return *static_cast<const TYPE*>(list().next()->data());  }
        __ABE_INLINE TYPE&       back()                  { return *static_cast<TYPE*>(list().prev()->data());        }
        __ABE_INLINE const TYPE& back() const            { return *static_cast<const TYPE*>(list().prev()->data());  }

    public:
        __ABE_INLINE void    push(const TYPE& v)         { ListImpl::push(ListImpl::list(), &v);         }
        __ABE_INLINE void    push_back(const TYPE& v)    { push(v);                                      }
        __ABE_INLINE void    push_front(const TYPE& v)   { ListImpl::push(*ListImpl::list().next(), &v); }

        __ABE_INLINE void    pop()                       { ListImpl::pop(*ListImpl::list().next());      }
        __ABE_INLINE void    pop_front()                 { pop();                                        }
        __ABE_INLINE void    pop_back()                  { ListImpl::pop(*ListImpl::list().prev());      }

    public:
        // emplace operations
        __ABE_INLINE TYPE&   push()                      { return *static_cast<TYPE*>(ListImpl::emplace(ListImpl::list(), type_construct<TYPE>).data());            }
        __ABE_INLINE TYPE&   push_back()                 { return push();                                                                                            }
        __ABE_INLINE TYPE&   push_front()                { return *static_cast<TYPE*>(ListImpl::emplace(*ListImpl::list().next(), type_construct<TYPE>).data());    }

    public:
        __ABE_INLINE iterator        begin()             { return iterator(ListImpl::list().next());         }
        __ABE_INLINE iterator        end()               { return iterator(&ListImpl::list());               }
        __ABE_INLINE iterator        rbegin()            { return iterator(ListImpl::list().prev());         }
        __ABE_INLINE iterator        rend()              { return iterator(&ListImpl::list());               }

        __ABE_INLINE const_iterator  cbegin() const      { return const_iterator(ListImpl::list().next());   }
        __ABE_INLINE const_iterator  cend() const        { return const_iterator(&ListImpl::list());         }
        __ABE_INLINE const_iterator  crbegin() const     { return const_iterator(ListImpl::list().prev());   }
        __ABE_INLINE const_iterator  crend() const       { return const_iterator(&ListImpl::list());         }

    public:
        __ABE_INLINE iterator    insert(iterator pos, const TYPE& v) { return iterator(&ListImpl::push(*pos.mNode, &v));   }
        __ABE_INLINE iterator    erase(iterator pos)                 { return iterator(&ListImpl::pop(*pos.mNode));        }
};

__END_NAMESPACE_ABE
#endif // __cplusplus
#endif // _TOOLKIT_HEADERS_STL_LIST_H 
