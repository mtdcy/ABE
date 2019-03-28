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

__BEGIN_DECLS

enum {
    OBJECT_ID_ANY,
    OBJECT_ID_ALLOCATOR,
    OBJECT_ID_RUNNABLE,
};
__END_DECLS

#ifdef __cplusplus

__BEGIN_NAMESPACE_ABE

/**
 * all members of SharedObject use static member function style start with capital
 * to avoid overload by subclass
 * @note client should NOT keep a pointer to SharedObject without retain, always
 *       retain first, use it later
 */
struct SharedObject {
    private:
        const uint32_t  mID;
        volatile int    mRefs;

    protected:
        SharedObject();
        SharedObject(const uint32_t id);
        virtual ~SharedObject();

        SharedObject(const SharedObject& rhs);
        SharedObject& operator=(const SharedObject& rhs);

    public:
        __ALWAYS_INLINE uint32_t    GetObjectID() const { return mID; }

        /**
         * retain this object by increase reference count
         * XXX: did subclass need to overload RetainObject() ?
         */
        SharedObject *  RetainObject();

        /**
         * release this object by decrease reference count
         * if reference count reach 0, this object will be deleted
         * @param keep  whether to keep the memory if this is the last ref
         * @return return old reference count
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
        __ALWAYS_INLINE bool    IsObjectShared() const      { return GetRetainCount() > 1; }
        __ALWAYS_INLINE bool    IsObjectNotShared() const   { return !IsObjectShared(); }
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
SharedObject *  SharedObjectRetain(SharedObject *);

/**
 * release a shared object
 */
void            SharedObjectRelease(SharedObject *);

/**
 * get a shared object retain count
 */
size_t          SharedObjectGetRetainCount(SharedObject *);
#define SharedObjectIsShared(s)     (SharedObjectGetRetainCount(s) > 1)
#define SharedObjectIsNotShared(s)  !SharedObjectIsShared(s)

/**
 * get a shread object id
 */
uint32_t        SharedObjectGetID(SharedObject *);

__END_DECLS

#ifdef __cplusplus

__BEGIN_NAMESPACE_ABE

#define COMPARE(_op_)                                                   \
    __ALWAYS_INLINE bool operator _op_ (const sp<T>& o) const {         \
        return mShared _op_ o.mShared;                                  \
    }                                                                   \
__ALWAYS_INLINE bool operator _op_ (const T* o) const {             \
    return mShared _op_ o;                                          \
}                                                                   \
template<typename U>                                                \
__ALWAYS_INLINE bool operator _op_ (const sp<U>& o) const {         \
    return mShared _op_ o.mShared;                                  \
}                                                                   \
template<typename U>                                                \
__ALWAYS_INLINE bool operator _op_ (const U* o) const {             \
    return mShared _op_ o;                                          \
}                                                                   \

template <class T> class sp {
    public:
        // constructors
        __ALWAYS_INLINE sp() : mShared(NULL) { }
        __ALWAYS_INLINE sp(T* rhs);
        __ALWAYS_INLINE sp(const sp<T>& rhs);

        template<typename U> __ALWAYS_INLINE sp(U* rhs);
        template<typename U> __ALWAYS_INLINE sp(const sp<U>& rhs);

        // destructors
        __ALWAYS_INLINE ~sp();

        // copy assignments
        __ALWAYS_INLINE sp& operator=(T* rhs);
        __ALWAYS_INLINE sp& operator=(const sp<T>& rhs);

        template<typename U> __ALWAYS_INLINE sp& operator=(U* rhs);
        template<typename U> __ALWAYS_INLINE sp& operator=(const sp<U>& rhs);

        // clear
        __ALWAYS_INLINE void clear();

    public:
        /**
         * compare the object pointer, not the object content
         */
        COMPARE(==);
        COMPARE(!=);

    public:
        // access
        __ALWAYS_INLINE  T*         operator->()        { return static_cast<T*>(mShared);          }
        __ALWAYS_INLINE  const T*   operator->() const  { return static_cast<const T*>(mShared);    }

        __ALWAYS_INLINE  T&         operator*()         { return *static_cast<T*>(mShared);         }
        __ALWAYS_INLINE  const T&   operator*() const   { return *static_cast<const T*>(mShared);   }

        __ALWAYS_INLINE  T*         get() const         { return static_cast<T*>(mShared);          }

    public:
        __ALWAYS_INLINE size_t      refsCount() const   { return mShared->GetRetainCount();         }

    private:
        template<typename U> friend class sp;
        T *                 mShared;
};
#undef COMPARE

///////////////////////////////////////////////////////////////////////////
template <typename T> sp<T>::sp(T* rhs) : mShared(rhs)
{
    if (mShared) { mShared->RetainObject(); }
}

template <typename T> sp<T>::sp(const sp<T>& rhs) : mShared(rhs.mShared)
{
    if (mShared) { mShared->RetainObject(); }
}

template<typename T> template<typename U> sp<T>::sp(U* rhs) :
    mShared(static_cast<T*>(rhs))
{
    if (mShared) { mShared->RetainObject(); }
}

template<typename T> template<typename U> sp<T>::sp(const sp<U>& rhs) :
    mShared(static_cast<T*>(rhs.mShared))
{
    if (mShared) { mShared->RetainObject(); }
}

template<typename T> sp<T>& sp<T>::operator=(T* rhs) {
    if (mShared == rhs) return *this;
    if (mShared)    mShared->ReleaseObject();
    mShared         = rhs;
    if (mShared)    mShared->RetainObject();
    return *this;
}

template<typename T> sp<T>& sp<T>::operator=(const sp<T>& rhs) {
    if (mShared == rhs.mShared) return *this;
    if (mShared)    mShared->ReleaseObject();
    mShared         = rhs.mShared;
    if (mShared)    mShared->RetainObject();
    return *this;
}

template<typename T> template<typename U> sp<T>& sp<T>::operator=(U* rhs) {
    if (mShared == rhs) return *this;
    if (mShared)    mShared->ReleaseObject();
    mShared         = static_cast<T*>(rhs);
    if (mShared)    mShared->RetainObject();
    return *this;
}

template<typename T> template<typename U> sp<T>& sp<T>::operator=(const sp<U>& rhs) {
    if (mShared == rhs.mShared) return *this;
    if (mShared)    mShared->ReleaseObject();
    mShared         = static_cast<T*>(rhs.mShared);
    if (mShared)    mShared->RetainObject();
    return *this;
}

template<typename T> sp<T>::~sp() { clear(); }
template<typename T> void sp<T>::clear() {
    if (mShared) {
        T * tmp = mShared;
        // clear mShared before release(), avoid loop in object destruction
        mShared = NULL;
        tmp->ReleaseObject();
    }
}

__END_NAMESPACE_ABE

#endif // __cplusplus

#endif // _TOOLKIT_HEADERS_OBJECT_H

