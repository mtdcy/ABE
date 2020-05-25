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


// File:    Types.h
// Author:  mtdcy.chen
// Changes:
//          1. 20160701     initial version
//

#ifndef ABE_HEADERS_TYPES_H
#define ABE_HEADERS_TYPES_H

/**
 * @note NO config.h for ABE public headers
 *
 * @note only system/libc/compiler macro allowed, like __APPLE__
 *  https://sourceforge.net/p/predef/wiki/Architectures/
 *  https://sourceforge.net/p/predef/wiki/Compilers/
 *  https://sourceforge.net/p/predef/wiki/OperatingSystems/
 *  https://blog.kowalczyk.info/article/j/guide-to-predefined-macros-in-c-compilers-gcc-clang-msvc-etc..html
 *
 * Compiler:
 *    _MSC_VER & __clang__ & __GNUC__
 * Platform:
 *    __APPLE__ & _WIN32 & _WIN64 & __MINGW32__ & __ANDROID__ & __linux__
 *
 *
 * @note we assume everything in bytes-order
 * @note NO Endian specific operations expect tool designed for this
 **/

#include <stdint.h>         // fixed width integer: int8_t int16_t int32_t ...
#include <stddef.h>         // size_t, null
#include <stdbool.h>        // true, false
#include <stdarg.h>         // va_list
#include <sys/types.h>      // other types: ssize_t
#include <inttypes.h>       // for PRId32/PRId64/...
#include <errno.h>          // errno, ...do NOT expose errno in your API, define your own or nothing.

// at least these types should be defined.
// uint8_t  int8_t
// uint16_t int16_t
// uint32_t int32_t
// uint64_t int64_t
// size_t ssize_t
// bool     true false
// null

// atomic
// FIXME: understand the memmodel
#ifndef ABE_ATOMIC_MEMMODEL
#define ABE_ATOMIC_MEMMODEL             __ATOMIC_SEQ_CST
#endif

// fence
#define atomic_fence()                  __atomic_thread_fence(ABE_ATOMIC_MEMMODEL)

#if defined(__GNUC__) || defined(__clang__)
// The ‘__atomic’ builtins can be used with any integral scalar or pointer type
// that is 1, 2, 4, or 8 bytes in length. 16-byte integral types are also allowed
// if ‘__int128’ (see __int128) is supported by the architecture.
#define ABE_ATOMIC_STORE(p, val)        __atomic_store_n(p, val, ABE_ATOMIC_MEMMODEL)
#define ABE_ATOMIC_LOAD(p)              __atomic_load_n(p, ABE_ATOMIC_MEMMODEL)
#define ABE_ATOMIC_EXCHANGE(p, val)     __atomic_exchange_n(p, val, ABE_ATOMIC_MEMMODEL)
// if *p0 == *p1: val => *p0 : return true; else: *p0 => *p1 : return false
#define ABE_ATOMIC_CAS(p0, p1, val)     __atomic_compare_exchange_n(p0, p1, val, false, ABE_ATOMIC_MEMMODEL, ABE_ATOMIC_MEMMODEL)    // compare and swap

#define ABE_ATOMIC_ADD(p, val)          __atomic_add_fetch(p, val, ABE_ATOMIC_MEMMODEL)
#define ABE_ATOMIC_SUB(p, val)          __atomic_sub_fetch(p, val, ABE_ATOMIC_MEMMODEL)
#define ABE_ATOMIC_AND(p, val)          __atomic_and_fetch(p, val, ABE_ATOMIC_MEMMODEL)
#define ABE_ATOMIC_OR(p, val)           __atomic_or_fetch(p, val, ABE_ATOMIC_MEMMODEL)
#define ABE_ATOMIC_XOR(p, val)          __atomic_xor_fetch(p, val, ABE_ATOMIC_MEMMODEL)
#define ABE_ATOMIC_NAND(p, val)         __atomic_nand_fetch(p, val, ABE_ATOMIC_MEMMODEL)

#define ABE_ATOMIC_FETCH_ADD(p, val)    __atomic_fetch_add(p, val, ABE_ATOMIC_MEMMODEL)
#define ABE_ATOMIC_FETCH_SUB(p, val)    __atomic_fetch_sub(p, val, ABE_ATOMIC_MEMMODEL)
#define ABE_ATOMIC_FETCH_AND(p, val)    __atomic_fetch_and(p, val, ABE_ATOMIC_MEMMODEL)
#define ABE_ATOMIC_FETCH_OR(p, val)     __atomic_fetch_or(p, val, ABE_ATOMIC_MEMMODEL)
#define ABE_ATOMIC_FETCH_XOR(p, val)    __atomic_fetch_xor(p, val, ABE_ATOMIC_MEMMODEL)
#define ABE_ATOMIC_FETCH_NAND(p, val)   __atomic_fetch_nand(p, val, ABE_ATOMIC_MEMMODEL)
#elif _MSC_VER
#error "TODO: ATOMIC is missing for MSVC"
#endif

// macros
#if !defined(__BEGIN_DECLS) || !defined(__END_DECLS)
#ifdef __cplusplus
#define __BEGIN_DECLS                   extern "C" {
#define __END_DECLS                     }
#else
#define __BEGIN_DECLS
#define __END_DECLS
#endif
#endif

#if defined(_MSC_VER)
#define ABE_INLINE                      inline
#ifdef BUILD_ABE_DLL
#define ABE_EXPORT                      __declspec(dllexport)
#else
#define ABE_EXPORT                      __declspec(dllimport)
#endif
#define ABE_DEPRECATED                  __declspec(deprecated)
#else
//#define __ABE_INLINE                    __attribute__ ((__always_inline__))
#define ABE_INLINE                      __attribute__ ((__visibility__("hidden"), __always_inline__)) inline
#define ABE_EXPORT                      __attribute__ ((__visibility__("default")))
#define ABE_DEPRECATED                  __attribute__ ((deprecated))
#endif

#if defined(_MSC_VER)
#define ABE_LIKELY(x)                   (x)
#define ABE_UNLIKELY(x)                 !(x)
#else
#define ABE_LIKELY(x)                   __builtin_expect(x, true)
#define ABE_UNLIKELY(x)                 __builtin_expect(x, false)
#endif

#ifdef __cplusplus
#define __BEGIN_NAMESPACE(x)            namespace x {
#define __END_NAMESPACE(x)              }
#define __USING_NAMESPACE(x)            using namespace x;

#define __NAMESPACE_ABE                 abe
#define __BEGIN_NAMESPACE_ABE           __BEGIN_NAMESPACE(__NAMESPACE_ABE)
#define __END_NAMESPACE_ABE             __END_NAMESPACE(__NAMESPACE_ABE)

#define __NAMESPACE_ABE_PRIVATE         abe_private
#define __BEGIN_NAMESPACE_ABE_PRIVATE   __BEGIN_NAMESPACE(__NAMESPACE_ABE) __BEGIN_NAMESPACE(__NAMESPACE_ABE_PRIVATE)
#define __END_NAMESPACE_ABE_PRIVATE     __END_NAMESPACE(__NAMESPACE_ABE_PRIVATE) __END_NAMESPACE(__NAMESPACE_ABE)

#define USING_NAMESPACE_ABE             __USING_NAMESPACE(__NAMESPACE_ABE)
#define USING_NAMESPACE_ABE_PRIVATE     __USING_NAMESPACE(__NAMESPACE_ABE::__NAMESPACE_ABE_PRIVATE)

// borrow from Android
// Put this in the private: declarations for a class to be uncopyable.
#define DISALLOW_COPY(TypeName)         TypeName(const TypeName&)

// Put this in the private: declarations for a class to be unassignable.
#define DISALLOW_ASSIGN(TypeName)       void operator=(const TypeName&)

// A macro to disallow the copy constructor and operator= functions
// This should be used in the private: declarations for a class
// In most case, copy constructor and operator= is not neccessary,
// and it may cause problem if you don't declare it or misuse it.
#define DISALLOW_EVILS(TypeName)            \
    private:                                \
    TypeName(const TypeName&);              \
    TypeName& operator=(const TypeName&)

#define DISALLOW_DYNAMIC(TypeName)          \
    private:                                \
    void *  operator new(size_t);           \
    void    operator delete(void *)

//=============================================================
// Object Type
__BEGIN_NAMESPACE_ABE

enum {      //-- XXX remove this XXX --//
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

// FIXME: limit T to basic types
template <typename T> class Atomic : public NonSharedObject {
    private:
        volatile T value;

    public:
        ABE_INLINE Atomic()                     { ABE_ATOMIC_STORE(&value, static_cast<T>(0));  }
        ABE_INLINE Atomic(const Atomic& rhs)    { ABE_ATOMIC_STORE(&value, rhs.load());         }
        ABE_INLINE Atomic(T _value)             { ABE_ATOMIC_STORE(&value, _value);             }

        ABE_INLINE void   store(T val)          { ABE_ATOMIC_STORE(&value, val);                }
        ABE_INLINE T      load() const          { return ABE_ATOMIC_LOAD(&value);               }
        ABE_INLINE T      exchange(T val)       { return ABE_ATOMIC_EXCHANGE(&value, val);      }
        ABE_INLINE bool   cas(T& to, T val)     { return ABE_ATOMIC_CAS(&value, &to, val);      }   // compare and swap

        ABE_INLINE T      operator++()          { return ABE_ATOMIC_ADD(&value, 1);             }   // pre-increment
        ABE_INLINE T      operator++(int)       { return ABE_ATOMIC_FETCH_ADD(&value, 1);       }   // post_increment
        ABE_INLINE T      operator--()          { return ABE_ATOMIC_SUB(&value, 1);             }
        ABE_INLINE T      operator--(int)       { return ABE_ATOMIC_FETCH_SUB(&value, 1);       }

        ABE_INLINE T      operator+=(T val)     { return ABE_ATOMIC_ADD(&value, val);           }
        ABE_INLINE T      operator-=(T val)     { return ABE_ATOMIC_SUB(&value, val);           }
        ABE_INLINE T      operator&=(T val)     { return ABE_ATOMIC_AND(&value, val);           }
        ABE_INLINE T      operator|=(T val)     { return ABE_ATOMIC_OR(&value, val);            }
        ABE_INLINE T      operator^=(T val)     { return ABE_ATOMIC_XOR(&value, val);           }

        ABE_INLINE T      fetch_add(T val)      { return ABE_ATOMIC_FETCH_ADD(&value, val);     }
        ABE_INLINE T      fetch_sub(T val)      { return ABE_ATOMIC_FETCH_SUB(&value, val);     }
        ABE_INLINE T      fetch_and(T val)      { return ABE_ATOMIC_FETCH_AND(&value, val);     }
        ABE_INLINE T      fetch_or(T val)       { return ABE_ATOMIC_FETCH_OR(&value, val);      }
        ABE_INLINE T      fetch_xor(T val)      { return ABE_ATOMIC_FETCH_XOR(&value, val);     }
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
#endif // __cplusplus

#endif // ABE_HEADERS_TYPES_H

