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

#ifndef _TOOLKIT_HEADERS_OBJECT_H
#define _TOOLKIT_HEADERS_OBJECT_H
#include <ABE/basic/Types.h>
#include <ABE/basic/Atomic.h>

__BEGIN_DECLS

enum {
    OBJECT_ID_ANY,
    OBJECT_ID_ALLOCATOR,
    OBJECT_ID_SHAREDBUFFER,
    OBJECT_ID_STRING,
    OBJECT_ID_BUFFER,
    OBJECT_ID_MESSAGE,
    OBJECT_ID_RUNNABLE,
    OBJECT_ID_LOOPER,
    OBJECT_ID_EVENT
};
__END_DECLS

#ifdef __cplusplus

__BEGIN_NAMESPACE_ABE

// https://stackoverflow.com/questions/6271615/any-way-to-prevent-dynamic-allocation-of-a-class
struct __ABE_EXPORT NonSharedObject {
    public:
        NonSharedObject() { }
        ~NonSharedObject() { }
    
    private:
        DISALLOW_DYNAMIC(NonSharedObject);
};

/**
 * all members of SharedObject use static member function style start with capital
 * to avoid overload by subclass
 * @note client should NOT keep a pointer to SharedObject without retain, always
 *       retain first, use it later
 */
struct __ABE_EXPORT SharedObject {
    private:
        const uint32_t  mID;
        Atomic<size_t>  mRefs;

    protected:
        SharedObject();
        SharedObject(const uint32_t id);
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
         * been called when last retain released
         * @note put code here to avoid 'Pure virtual function called!'.
         */
        virtual void    onLastRetain() { }

    public:
        __ABE_INLINE uint32_t    GetObjectID() const { return mID; }

        /**
         * retain this object by increase reference count
         * XXX: did subclass need to overload RetainObject() ?
         */
        SharedObject *  RetainObject();

        /**
         * release this object by decrease reference count
         * if reference count reach 0, this object will be deleted
         * @param keep  whether to keep the memory if this is the last ref
         * @return return new reference count
         * @note keep the memory if subclass need to do extra destruction work
         */
        size_t          ReleaseObject(bool keep = false);

        /**
         * get this object reference count
         * @return return current reference count
         */
        size_t          GetRetainCount() const;

        /**
         * is this object shared with others
         * @note if object is not shared, it is safe to do anything
         *       else either copy this object or lock it to modify its context
         */
        __ABE_INLINE bool    IsObjectShared() const      { return GetRetainCount() > 1; }
        __ABE_INLINE bool    IsObjectNotShared() const   { return !IsObjectShared(); }
    
    private:
        DISALLOW_EVILS(SharedObject);
};

__END_NAMESPACE_ABE

#endif   // __cplusplus

#ifdef __cplusplus
using __NAMESPACE_ABE::SharedObject;
#else
typedef struct SharedObject SharedObject;
#endif

__BEGIN_DECLS

/**
 * retain a shared object
 */
__ABE_EXPORT SharedObject *  SharedObjectRetain(SharedObject *);

/**
 * release a shared object
 */
__ABE_EXPORT void            SharedObjectRelease(SharedObject *);

/**
 * get a shared object retain count
 */
__ABE_EXPORT size_t          SharedObjectGetRetainCount(SharedObject *);
#define SharedObjectIsShared(s)     (SharedObjectGetRetainCount(s) > 1)
#define SharedObjectIsNotShared(s)  !SharedObjectIsShared(s)

/**
 * get a shread object id
 */
__ABE_EXPORT uint32_t        SharedObjectGetID(SharedObject *);

__END_DECLS

#ifdef __cplusplus

__BEGIN_NAMESPACE_ABE

#define COMPARE(_op_)                                                   \
    __ABE_INLINE bool operator _op_ (const Object<T>& o) const {        \
        return mShared _op_ o.mShared;                                  \
    }                                                                   \
    __ABE_INLINE bool operator _op_ (const T* o) const {                \
        return mShared _op_ o;                                          \
    }                                                                   \
    template<typename U>                                                \
    __ABE_INLINE bool operator _op_ (const Object<U>& o) const {        \
        return mShared _op_ o.mShared;                                  \
    }                                                                   \
    template<typename U>                                                \
    __ABE_INLINE bool operator _op_ (const U* o) const {                \
        return mShared _op_ o;                                          \
    }                                                                   \

template <class T> class __ABE_EXPORT Object {
    public:
        // constructors
        __ABE_INLINE Object() : mShared(NULL) { }
        __ABE_INLINE Object(SharedObject * rhs)                         { set(rhs);             }
        __ABE_INLINE Object(const Object<T>& rhs)                       { set(rhs.mShared);     }
        template<typename U> __ABE_INLINE Object(U * rhs)               { set(rhs);             }
        template<typename U> __ABE_INLINE Object(const Object<U>& rhs)  { set(rhs.mShared);     }

        // destructors
        __ABE_INLINE ~Object() { clear(); }

        // copy assignments
        __ABE_INLINE Object& operator=(SharedObject * rhs)                          { clear(); set(rhs); return *this;          }
        __ABE_INLINE Object& operator=(const Object<T>& rhs)                        { clear(); set(rhs.mShared); return *this;  }
        template<typename U> __ABE_INLINE Object& operator=(U * rhs)                { clear(); set(rhs); return *this;          }
        template<typename U> __ABE_INLINE Object& operator=(const Object<U>& rhs)   { clear(); set(rhs.mShared); return *this;  }

        // clear
        __ABE_INLINE void clear();

    public:
        /**
         * compare the object pointer, not the object content
         */
        COMPARE(==);
        COMPARE(!=);

    public:
        // access
        __ABE_INLINE  T*         operator->()       { return static_cast<T*>(mShared);          }
        __ABE_INLINE  const T*   operator->() const { return static_cast<const T*>(mShared);    }

        __ABE_INLINE  T&         operator*()        { return *static_cast<T*>(mShared);         }
        __ABE_INLINE  const T&   operator*() const  { return *static_cast<const T*>(mShared);   }

        __ABE_INLINE  T*         get() const        { return static_cast<T*>(mShared);          }

    public:
        __ABE_INLINE size_t      refsCount() const  { return mShared->GetRetainCount();         }

    private:
        DISALLOW_DYNAMIC(Object);
    
        template<typename U> friend class Object;
        __ABE_INLINE void                       set(SharedObject *);
        template<typename U> __ABE_INLINE void  set(U * );
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

#endif // __cplusplus

#endif // _TOOLKIT_HEADERS_OBJECT_H

