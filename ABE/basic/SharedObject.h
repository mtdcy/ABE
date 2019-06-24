/******************************************************************************
 * Copyright (c) 2018, Chen Fang <mtdcy.chen@gmail.com>
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

// File:    SharedObject.h
// Author:  mtdcy.chen
// Changes: 
//          1. 20181112     initial version
//

#ifndef ABE_HEADERS_OBJECT_H
#define ABE_HEADERS_OBJECT_H
#include <ABE/basic/Types.h>
#include <ABE/basic/Atomic.h>

__BEGIN_NAMESPACE_ABE

enum {
    OBJECT_ID_ANY,
    OBJECT_ID_ALLOCATOR,
    OBJECT_ID_SHAREDBUFFER,
    OBJECT_ID_STRING,
    OBJECT_ID_BUFFER,
    OBJECT_ID_MESSAGE,
    OBJECT_ID_RUNNABLE,
    OBJECT_ID_LOOPER,
    OBJECT_ID_EVENT,
    OBJECT_ID_MAX   = INT32_MAX
};

// https://stackoverflow.com/questions/6271615/any-way-to-prevent-dynamic-allocation-of-a-class
/**
 * object can NOT be shared
 * @note usally for local accessory class
 */
struct ABE_EXPORT NonSharedObject {
    NonSharedObject() { }
    ~NonSharedObject() { }
    
    DISALLOW_DYNAMIC(NonSharedObject);
};

/**
 * all members of SharedObject use static member function style start with capital
 * to avoid overload by subclass
 * @note client should NOT keep a pointer to SharedObject without retain, always
 *       retain first, use it later
 */
struct ABE_EXPORT SharedObject {
    private:
        const uint32_t  mID;
        Atomic<size_t>  mRefs;

    protected:
        SharedObject(const uint32_t id = OBJECT_ID_ANY);
        /**
         * @note it is a good practice to leave virtual destruction empty
         */
        virtual ~SharedObject() { }
    
    protected:
        /**
         * been called when first retain the object
         */
        virtual void    onFirstRetain() { }
        /**
         * been called when last retain released and before delete memory
         * @note put code here to avoid 'Pure virtual function called!'.
         */
        virtual void    onLastRetain() { }

    public:
        ABE_INLINE uint32_t GetObjectID() const { return mID; }
    
        /**
         * get this object reference count
         * @return return current reference count
         */
        ABE_INLINE size_t   GetRetainCount() const { return mRefs.load(); }

        /**
         * retain this object by increase reference count
         * XXX: did subclass need to overload RetainObject() ?
         */
        SharedObject *      RetainObject();

        /**
         * release this object by decrease reference count
         * if reference count reach 0, this object will be deleted
         * @param keep  whether to keep the memory if this is the last ref
         * @return return new reference count
         * @note keep the memory if subclass need to do extra destruction work
         */
        size_t              ReleaseObject(bool keep = false);

        /**
         * is this object shared with others
         * @note if object is not shared, it is safe to do anything
         *       else either copy this object or lock it to modify its context
         */
        ABE_INLINE bool    IsObjectShared() const      { return GetRetainCount() > 1; }
        ABE_INLINE bool    IsObjectNotShared() const   { return !IsObjectShared(); }
    
    DISALLOW_EVILS(SharedObject);
};

#define COMPARE(_op_)                                                   \
    ABE_INLINE bool operator _op_ (const Object<T>& o) const {        \
        return mShared _op_ o.mShared;                                  \
    }                                                                   \
    ABE_INLINE bool operator _op_ (const T* o) const {                \
        return mShared _op_ o;                                          \
    }                                                                   \
    template<typename U>                                                \
    ABE_INLINE bool operator _op_ (const Object<U>& o) const {        \
        return mShared _op_ o.mShared;                                  \
    }                                                                   \
    template<typename U>                                                \
    ABE_INLINE bool operator _op_ (const U* o) const {                \
        return mShared _op_ o;                                          \
    }                                                                   \

/**
 * SharedObject accessory
 * help retain & release SharedObject
 */
template <class T> class ABE_EXPORT Object : public NonSharedObject {
    public:
        // constructors
        ABE_INLINE Object() : mShared(NULL) { }
        ABE_INLINE Object(SharedObject * rhs)                         { set(rhs);             }
        ABE_INLINE Object(const Object<T>& rhs)                       { set(rhs.mShared);     }
        template<typename U> ABE_INLINE Object(U * rhs)               { set(rhs);             }
        template<typename U> ABE_INLINE Object(const Object<U>& rhs)  { set(rhs.mShared);     }

        // destructors
        ABE_INLINE ~Object() { clear(); }

        // copy assignments
        ABE_INLINE Object& operator=(SharedObject * rhs)                          { clear(); set(rhs); return *this;          }
        ABE_INLINE Object& operator=(const Object<T>& rhs)                        { clear(); set(rhs.mShared); return *this;  }
        template<typename U> ABE_INLINE Object& operator=(U * rhs)                { clear(); set(rhs); return *this;          }
        template<typename U> ABE_INLINE Object& operator=(const Object<U>& rhs)   { clear(); set(rhs.mShared); return *this;  }

        // clear
        ABE_INLINE void clear();
    
        ABE_INLINE bool isNIL() const { return mShared == NULL ; }

    public:
        /**
         * compare the object pointer, not the object content
         */
        COMPARE(==);
        COMPARE(!=);

    public:
        // access
        ABE_INLINE  T*         operator->()       { return static_cast<T*>(mShared);          }
        ABE_INLINE  const T*   operator->() const { return static_cast<const T*>(mShared);    }

        ABE_INLINE  T&         operator*()        { return *static_cast<T*>(mShared);         }
        ABE_INLINE  const T&   operator*() const  { return *static_cast<const T*>(mShared);   }

        ABE_INLINE  T*         get() const        { return static_cast<T*>(mShared);          }

    public:
        ABE_INLINE size_t      refsCount() const  { return mShared->GetRetainCount();         }
    
        template<typename U> friend class Object;
        ABE_INLINE void                       set(SharedObject *);
        template<typename U> ABE_INLINE void  set(U * );
        SharedObject *  mShared;
};
#undef COMPARE

///////////////////////////////////////////////////////////////////////////
template <typename T> void Object<T>::set(SharedObject * shared) {
    mShared = shared;
    if (mShared) { mShared->RetainObject(); }
}

template<typename T> template<typename U> void Object<T>::set(U * shared) {
    mShared = static_cast<SharedObject *>(shared);
    if (mShared) { mShared->RetainObject(); }
}

template<typename T> void Object<T>::clear() {
    if (mShared) {
        SharedObject * tmp = mShared;
        // clear mShared before release(), avoid loop in object destruction
        mShared = NULL;
        tmp->ReleaseObject();
    }
}

__END_NAMESPACE_ABE

#endif // ABE_HEADERS_OBJECT_H

