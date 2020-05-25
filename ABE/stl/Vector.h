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


// File:    Vector.h
// Author:  mtdcy.chen
// Changes: 
//          1. 20160829     initial version
//


#ifndef ABE_HEADERS_STL_VECTOR_H
#define ABE_HEADERS_STL_VECTOR_H 

#include <ABE/stl/TypeHelper.h>
#include <ABE/core/Allocator.h>
#include <ABE/core/SharedBuffer.h>

__BEGIN_NAMESPACE_ABE_PRIVATE
class ABE_EXPORT VectorImpl {
    public:
        VectorImpl(const Object<Allocator>& allocator,
                size_t capacity, const TypeHelper& helper);

        VectorImpl(const VectorImpl& rhs);

        ~VectorImpl();

        VectorImpl& operator=(const VectorImpl& rhs);

    protected:
        void        clear();
        void        shrink();

        ABE_INLINE size_t      size() const        { return mItemCount; }
        ABE_INLINE size_t      capacity() const    { return mCapacity; }

    protected:
        const void* access  (size_t index) const;
        void *      access  (size_t index);

        void *      emplace (size_t pos, type_construct_t ctor);
        void        insert  (size_t pos, const void * what);
        void        erase   (size_t pos);
        void        erase   (size_t first, size_t last);

        void        sort    (type_compare_t);

    private:
        char *      _grow   (size_t pos, size_t amount);
        void        _remove (size_t pos, size_t amount);
        void        _edit   ();
        void        _release(SharedBuffer *, size_t, bool moved = false);

    private:
        TypeHelper          mTypeHelper;
        Object<Allocator>   mAllocator;
        SharedBuffer *      mStorage;
        size_t              mCapacity;
        size_t              mItemCount;
};
__END_NAMESPACE_ABE_PRIVATE

__BEGIN_NAMESPACE_ABE
// Note:
// 1. No interator for Vector because it's random access has constant time.
// 2. No auto memory shrink
template <typename TYPE> class Vector : protected __NAMESPACE_ABE_PRIVATE::VectorImpl, public NonSharedObject {
    public:
        ABE_INLINE Vector(size_t capacity = 4, const Object<Allocator>& allocator = kAllocatorDefault) :
            VectorImpl(allocator, capacity, TypeHelperBuilder<TYPE, false, true, true>()) { }

        ABE_INLINE ~Vector() { }

    public:
        ABE_INLINE void        clear()             { VectorImpl::clear();              }
        ABE_INLINE void        shrink()            { VectorImpl::shrink();             }
        ABE_INLINE size_t      size() const        { return VectorImpl::size();        }
        ABE_INLINE bool        empty() const       { return size() == 0;               }
        ABE_INLINE size_t      capacity() const    { return VectorImpl::capacity();    }

    public:
        // stable sort, uses operator<
        ABE_INLINE void        sort()              { VectorImpl::sort(type_compare_less<TYPE>);                }
        typedef     bool compare_t (const TYPE *lhs, const TYPE *rhs);
        ABE_INLINE void        sort(compare_t cmp) { VectorImpl::sort(reinterpret_cast<type_compare_t>(cmp));  }

    public:
        // element access with range check which is not like std::vector::operator[]
        ABE_INLINE TYPE&       operator[](size_t index)        { return *static_cast<TYPE*>(access(index));        }
        ABE_INLINE const TYPE& operator[](size_t index) const  { return *static_cast<const TYPE*>(access(index));  }

    public:
        ABE_INLINE TYPE&       front()             { return *static_cast<TYPE*>(VectorImpl::access(0));                }
        ABE_INLINE TYPE&       back()              { return *static_cast<TYPE*>(VectorImpl::access(size()-1));         }
        ABE_INLINE const TYPE& front() const       { return *static_cast<const TYPE*>(VectorImpl::access(0));          }
        ABE_INLINE const TYPE& back() const        { return *static_cast<const TYPE*>(VectorImpl::access(size()-1));   }

    public:
        ABE_INLINE void        push(const TYPE& v) { VectorImpl::insert(size(), &v);                                                   }
        ABE_INLINE TYPE&       push()              { return *static_cast<TYPE*>(VectorImpl::emplace(size(), type_construct<TYPE>));    }
        ABE_INLINE void        pop()               { VectorImpl::erase(size()-1);                                                      }

    public:
        ABE_INLINE void        erase(size_t index)                 { VectorImpl::erase(index);                                 }
        ABE_INLINE void        erase(size_t first, size_t last)    { VectorImpl::erase(first, last);                           }
        ABE_INLINE void        insert(size_t index, const TYPE& v) { VectorImpl::insert(index, &v);                            }
        ABE_INLINE TYPE&       insert(size_t index)                { return *static_cast<TYPE*>(VectorImpl::emplace(index, type_construct<TYPE>));  }
};

__END_NAMESPACE_ABE
#endif // ABE_HEADERS_STL_VECTOR_H 
