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

#ifndef ABE_HEADERS_HT_H
#define ABE_HEADERS_HT_H

#include <ABE/stl/TypeHelper.h>
#include <ABE/stl/Vector.h>

__BEGIN_NAMESPACE_ABE_PRIVATE

class ABE_EXPORT HashTableImpl {
    public:
        HashTableImpl(const sp<Allocator>& allocator,
                UInt32 tableLength,
                const TypeHelper& keyHelper,
                const TypeHelper& valueHelper,
                type_compare_t keyCompare);
        HashTableImpl(const HashTableImpl&);
        HashTableImpl& operator=(const HashTableImpl&);
        ~HashTableImpl();

    protected:
        struct Element {
            const UInt32    mHash;
            void *          mKey;
            void *          mValue;
            Element *       mNext;

            Element(UInt32);
            ~Element();
        };

    protected:
        ABE_INLINE UInt32   size () const { return mNumElements; }

    protected:
        void                insert      (const void *k, const void *v, UInt32 hash);
        UInt32              erase       (const void *k, UInt32 hash);
        void                clear       ();

    protected:
        // return Nil if not exists
        void *              find        (const void *k, UInt32 hash);
        const void *        find        (const void *k, UInt32 hash) const;
        // assert if not exists
        void *              access      (const void *k, UInt32 hash);
        const void *        access      (const void *k, UInt32 hash) const;

    protected:
        // for iterator
        ABE_INLINE UInt32   tableLength() const { return mTableLength; }
        Element *           next        (const Element *, UInt32 *);
        const Element *     next        (const Element *, UInt32 *) const;

    private:
        Element *           allocateElement     (UInt32);
        void                deallocateElement   (Element *);
        void                grow        ();
        void                shrink      ();

    private:
        Element **          _edit();
        void                _release(SharedBuffer *);

    private:
        TypeHelper          mKeyHelper;
        TypeHelper          mValueHelper;
        type_compare_t      mKeyCompare;  // make sure element is unique
        sp<Allocator>       mAllocator;
        SharedBuffer *      mStorage;
        UInt32              mTableLength;
        UInt32              mNumElements;
};

__END_NAMESPACE_ABE_PRIVATE

__BEGIN_NAMESPACE_ABE
//////////////////////////////////////////////////////////////////////////////
// implementation of hash of core types
template <typename TYPE> static ABE_INLINE UInt32 hash(const TYPE& value) {
    return value.hash();
};

#define HASH_BASIC_TYPES32(TYPE)                                                        \
    template <> ABE_INLINE UInt32 hash(const TYPE& v) { return UInt32(v); }
#define HASH_BASIC_TYPES64(TYPE)                                                        \
    template <> ABE_INLINE UInt32 hash(const TYPE& v) { return UInt32((v >> 32) ^ v); }
#define HASH_BASIC_TYPES(TYPE)                                                          \
    template <> ABE_INLINE UInt32 hash(const TYPE& v) {                               \
        UInt32 x = 0;                                                                   \
        const UInt8 *u8 = reinterpret_cast<const UInt8*>(&v);                       \
        for (UInt32 i = 0; i < sizeof(TYPE); ++i) x = x * 31 + u8[i];                   \
        return x;                                                                       \
    };

HASH_BASIC_TYPES32  (UInt8);
HASH_BASIC_TYPES32  (Int8);
HASH_BASIC_TYPES32  (UInt16);
HASH_BASIC_TYPES32  (Int16);
HASH_BASIC_TYPES32  (UInt32);
HASH_BASIC_TYPES32  (Int32);
HASH_BASIC_TYPES64  (UInt64);
HASH_BASIC_TYPES64  (Int64);
HASH_BASIC_TYPES    (Float32);
HASH_BASIC_TYPES    (Float64);
#undef HASH_BASIC_TYPES
#undef HASH_BASIC_TYPES32
#undef HASH_BASIC_TYPES64

template <typename TYPE> ABE_INLINE UInt32 hash(TYPE * const& p) {
    return hash<uintptr_t>(uintptr_t(p));
};

template <typename KEY, typename VALUE> class HashTable : private __NAMESPACE_PRIVATE::HashTableImpl, public StaticObject {
    private:
        // increment only iterator
        template <class TABLE_TYPE, class VALUE_TYPE, class ELEM_TYPE> class Iterator {
            public:
                ABE_INLINE Iterator() : mTable(Nil), mIndex(0), mElement(Nil) { }
                ABE_INLINE Iterator(TABLE_TYPE table, UInt32 index, const ELEM_TYPE& e) : mTable(table), mIndex(index), mElement(e) { }
                ABE_INLINE ~Iterator() { }

                ABE_INLINE Iterator&   operator++()    { next(); return *this;                                 }   // pre-increment
                ABE_INLINE Iterator    operator++(int) { Iterator old(*this); next(); return old;              }   // post-increment

                ABE_INLINE Bool        operator == (const Iterator& rhs) const { return mElement == rhs.mElement; }
                ABE_INLINE Bool        operator != (const Iterator& rhs) const { return !operator==(rhs);      }

                ABE_INLINE const KEY&  key() const     { return *static_cast<KEY*>(mElement->mKey);            }
                ABE_INLINE VALUE_TYPE& value()         { return *static_cast<VALUE_TYPE*>(mElement->mValue);   }

            private:
                ABE_INLINE void        next()          { mElement = mTable->next(mElement, &mIndex);           }

            protected:
                TABLE_TYPE  mTable;
                UInt32      mIndex;
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
        ABE_INLINE HashTable(UInt32 tableLength = 4, const sp<Allocator>& allocator = kAllocatorDefault) :
            HashTableImpl(allocator, tableLength,
                    TypeHelperBuilder<KEY, False, True, False>(),
                    TypeHelperBuilder<VALUE, False, True, False>(),
                    type_compare_equal<KEY>) { }

        ABE_INLINE ~HashTable() { }

    public:
        ABE_INLINE UInt32           size() const        { return HashTableImpl::size();         }
        ABE_INLINE Bool             empty() const       { return size() == 0;                   }
        ABE_INLINE void             clear()             { HashTableImpl::clear();               }

    public:
        // insert value with key, replace if exists
        ABE_INLINE void             insert(const KEY& k, const VALUE& v){ HashTableImpl::insert(&k, &v, hash(k));                                   }
        // erase element with key, return 1 if exists, and 0 otherwise.
        ABE_INLINE UInt32           erase(const KEY& k)                 { return HashTableImpl::erase(&k, hash(k));                                 }
        // return Nil if not exists
        ABE_INLINE VALUE *          find(const KEY& k)                  { return static_cast<VALUE*>(HashTableImpl::find(&k, hash(k)));             }
        ABE_INLINE const VALUE*     find(const KEY& k) const            { return static_cast<const VALUE*>(HashTableImpl::find(&k, hash(k)));       }
        // assert if not exists
        ABE_INLINE VALUE&           operator[](const KEY& k)            { return *static_cast<VALUE*>(HashTableImpl::access(&k, hash(k)));          }
        ABE_INLINE const VALUE&     operator[](const KEY& k) const      { return *static_cast<const VALUE*>(HashTableImpl::access(&k, hash(k)));    }

    public:
        // forward iterator
        ABE_INLINE iterator         begin()         { return ++iterator(this, 0, Nil);                                 }
        ABE_INLINE iterator         end()           { return iterator(this, HashTableImpl::tableLength(), Nil);        }
        ABE_INLINE const_iterator   cbegin() const  { return ++const_iterator(this, 0, Nil);                           }
        ABE_INLINE const_iterator   cend() const    { return const_iterator(this, HashTableImpl::tableLength(), Nil);  }
};

__END_NAMESPACE_ABE

#endif // ABE_HEADERS_HT_H

