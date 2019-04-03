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


// File:    HashTable.h
// Author:  mtdcy.chen
// Changes:
//          1. 20160701     initial version
//

#ifndef _TOOLKIT_HEADERS_HT_H
#define _TOOLKIT_HEADERS_HT_H

#ifdef __cplusplus

#include <ABE/stl/TypeHelper.h>
#include <ABE/stl/Vector.h>

__BEGIN_NAMESPACE_ABE_PRIVATE

class __ABE_EXPORT HashTableImpl {
    public:
        HashTableImpl(const sp<Allocator>& allocator,
                size_t tableLength,
                const TypeHelper& keyHelper,
                const TypeHelper& valueHelper,
                type_compare_t keyCompare);
        HashTableImpl(const HashTableImpl&);
        HashTableImpl& operator=(const HashTableImpl&);
        ~HashTableImpl();

    protected:
        struct __ABE_HIDDEN Element {
            const size_t    mHash;
            void *          mKey;
            void *          mValue;
            Element *       mNext;

            Element(size_t);
            ~Element();
        };

    protected:
        __ABE_INLINE size_t size () const { return mNumElements; }

    protected:
        void            insert      (const void *k, const void *v, size_t hash);
        size_t          erase       (const void *k, size_t hash);
        void            clear       ();

    protected:
        // return NULL if not exists
        void *          find        (const void *k, size_t hash);
        const void *    find        (const void *k, size_t hash) const;
        // assert if not exists
        void *          access      (const void *k, size_t hash);
        const void *    access      (const void *k, size_t hash) const;

    protected:
        // for iterator
        __ABE_INLINE size_t tableLength() const { return mTableLength; }
        Element *       next        (const Element *, size_t *);
        const Element * next        (const Element *, size_t *) const;

    private:
        Element *       allocateElement     (size_t);
        void            deallocateElement   (Element *);
        void            grow        ();
        void            shrink      ();

    private:
        Element **      _edit();
        void            _release(SharedBuffer *);

    private:
        TypeHelper      mKeyHelper;
        TypeHelper      mValueHelper;
        type_compare_t  mKeyCompare;  // make sure element is unique
        sp<Allocator>   mAllocator;
        SharedBuffer *  mStorage;
        size_t          mTableLength;
        size_t          mNumElements;
};

__END_NAMESPACE_ABE_PRIVATE

__BEGIN_NAMESPACE_ABE
//////////////////////////////////////////////////////////////////////////////
// implementation of hash of basic types
template <typename TYPE> static __ABE_INLINE size_t hash(const TYPE& value) {
    return value.hash();
};

#define HASH_BASIC_TYPES32(TYPE)                                                        \
    template <> __ABE_INLINE size_t hash(const TYPE& v) { return size_t(v); }
#define HASH_BASIC_TYPES64(TYPE)                                                        \
    template <> __ABE_INLINE size_t hash(const TYPE& v) { return size_t((v >> 32) ^ v); }
#define HASH_BASIC_TYPES(TYPE)                                                          \
    template <> __ABE_INLINE size_t hash(const TYPE& v) {                               \
        size_t x = 0;                                                                   \
        const uint8_t *u8 = reinterpret_cast<const uint8_t*>(&v);                       \
        for (size_t i = 0; i < sizeof(TYPE); ++i) x = x * 31 + u8[i];                   \
        return x;                                                                       \
    };

HASH_BASIC_TYPES32  (uint8_t);
HASH_BASIC_TYPES32  (int8_t);
HASH_BASIC_TYPES32  (uint16_t);
HASH_BASIC_TYPES32  (int16_t);
HASH_BASIC_TYPES32  (uint32_t);
HASH_BASIC_TYPES32  (int32_t);
HASH_BASIC_TYPES64  (uint64_t);
HASH_BASIC_TYPES64  (int64_t);
HASH_BASIC_TYPES    (float);
HASH_BASIC_TYPES    (double);

#if !defined(__GLIBC__) && !defined(__MINGW32__)
HASH_BASIC_TYPES32(size_t);
HASH_BASIC_TYPES32(ssize_t);
#endif
#undef HASH_BASIC_TYPES
#undef HASH_BASIC_TYPES32
#undef HASH_BASIC_TYPES64

template <typename TYPE> __ABE_INLINE size_t hash(TYPE * const& p) {
    return hash<uintptr_t>(uintptr_t(p));
};

template <typename KEY, typename VALUE> class HashTable : private __NAMESPACE_ABE_PRIVATE::HashTableImpl {
    private:
        // increment only iterator
        template <class TABLE_TYPE, class VALUE_TYPE, class ELEM_TYPE> class Iterator {
            public:
                __ABE_INLINE Iterator() : mTable(NULL), mIndex(0), mElement(NULL) { }
                __ABE_INLINE Iterator(TABLE_TYPE table, size_t index, const ELEM_TYPE& e) : mTable(table), mIndex(index), mElement(e) { }
                __ABE_INLINE ~Iterator() { }

                __ABE_INLINE Iterator&   operator++()    { next(); return *this;                                 }   // pre-increment
                __ABE_INLINE Iterator    operator++(int) { Iterator old(*this); next(); return old;              }   // post-increment

                __ABE_INLINE bool        operator == (const Iterator& rhs) const { return mElement == rhs.mElement; }
                __ABE_INLINE bool        operator != (const Iterator& rhs) const { return !operator==(rhs);      }

                __ABE_INLINE const KEY&  key() const     { return *static_cast<KEY*>(mElement->mKey);            }
                __ABE_INLINE VALUE_TYPE& value()         { return *static_cast<VALUE_TYPE*>(mElement->mValue);   }

            private:
                __ABE_INLINE void        next()          { mElement = mTable->next(mElement, &mIndex);           }

            protected:
                TABLE_TYPE  mTable;
                size_t      mIndex;
                ELEM_TYPE   mElement;

            private:
                // no decrement
                Iterator&   operator--();
                Iterator    operator--(int);
        };

    public:
        typedef Iterator<HashTable<KEY, VALUE> *, VALUE, Element *> iterator;
        typedef Iterator<const HashTable<KEY, VALUE> *, const VALUE, const Element *> const_iterator;

    public:
        __ABE_INLINE HashTable(size_t tableLength = 4, const sp<Allocator>& allocator = kAllocatorDefault) :
            HashTableImpl(allocator, tableLength,
                    TypeHelperBuilder<KEY, false, true, false>(),
                    TypeHelperBuilder<VALUE, false, true, false>(),
                    type_compare_equal<KEY>) { }

        __ABE_INLINE ~HashTable() { }

    public:
        __ABE_INLINE size_t          size() const        { return HashTableImpl::size();         }
        __ABE_INLINE bool     empty() const       { return size() == 0;                   }
        __ABE_INLINE void            clear()             { HashTableImpl::clear();               }

    public:
        // insert value with key, replace if exists
        __ABE_INLINE void            insert(const KEY& k, const VALUE& v){ HashTableImpl::insert(&k, &v, hash(k));                                   }
        // erase element with key, return 1 if exists, and 0 otherwise.
        __ABE_INLINE size_t          erase(const KEY& k)                 { return HashTableImpl::erase(&k, hash(k));                                 }
        // return NULL if not exists
        __ABE_INLINE VALUE *         find(const KEY& k)                  { return static_cast<VALUE*>(HashTableImpl::find(&k, hash(k)));             }
        __ABE_INLINE const VALUE*    find(const KEY& k) const            { return static_cast<const VALUE*>(HashTableImpl::find(&k, hash(k)));       }
        // assert if not exists
        __ABE_INLINE VALUE&          operator[](const KEY& k)            { return *static_cast<VALUE*>(HashTableImpl::access(&k, hash(k)));          }
        __ABE_INLINE const VALUE&    operator[](const KEY& k) const      { return *static_cast<const VALUE*>(HashTableImpl::access(&k, hash(k)));    }

    public:
        // iterator
        __ABE_INLINE iterator        begin()         { return ++iterator(this, 0, NULL);                                 }
        __ABE_INLINE iterator        end()           { return iterator(this, HashTableImpl::tableLength(), NULL);        }
        __ABE_INLINE const_iterator  cbegin() const  { return ++const_iterator(this, 0, NULL);                           }
        __ABE_INLINE const_iterator  cend() const    { return const_iterator(this, HashTableImpl::tableLength(), NULL);  }
};

__END_NAMESPACE_ABE
#endif // __cplusplus
#endif // _TOOLKIT_HEADERS_HT_H

