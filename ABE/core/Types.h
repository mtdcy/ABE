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

#pragma mark Headers
#include <stdint.h>
#include <stdarg.h>         // va_list
#include <inttypes.h>       // for PRId32/PRId64/...
#include <stddef.h>         // size_t, need by some internal apis
#define PRIf32  "f"
#define PRIf64  "lf"

#pragma mark Basic Types
/**
 * Basic Types
 * @note we discourage the use of size_t/Int
 */
// platform-depended width integer type
typedef int             Int;
typedef unsigned int    UInt;
// fixed-width unsigned integer type
typedef uint8_t         UInt8;
typedef uint16_t        UInt16;
typedef uint32_t        UInt32;
typedef uint64_t        UInt64;
// fixed-width signed integer type
typedef int8_t          Int8;
typedef int16_t         Int16;
typedef int32_t         Int32;
typedef int64_t         Int64;
// other types
typedef float           Float32;
typedef double          Float64;;
typedef char            Char;
// alias types
typedef Int             Bool;
typedef UInt32          Status;
// type values
#define True            1
#define False           0
#define Nil             0
#define OK              0   // For Status with no error

#pragma mark Macros
#define FOURCC(x) ((((UInt32)(x) >> 24) & 0xff)         \
                | (((UInt32)(x) >> 8) & 0xff00)         \
                | (((UInt32)(x) << 8) & 0xff0000)       \
                | (((UInt32)(x) << 24) & 0xff000000))

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
// if *p0 == *p1: val => *p0 : return True; else: *p0 => *p1 : return False
#define ABE_ATOMIC_CAS(p0, p1, val)     __atomic_compare_exchange_n(p0, p1, val, False, ABE_ATOMIC_MEMMODEL, ABE_ATOMIC_MEMMODEL)    // compare and swap

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
#ifdef __cplusplus
#define BEGIN_DECLS                     extern "C" {
#define END_DECLS                       }
#else
#define BEGIN_DECLS
#define END_DECLS
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
#define ABE_LIKELY(x)                   __builtin_expect(x, True)
#define ABE_UNLIKELY(x)                 __builtin_expect(x, False)
#endif

#ifdef __cplusplus
#define __BEGIN_NAMESPACE(x)            namespace x {
#define __END_NAMESPACE(x)              }
#define __USING_NAMESPACE(x)            using namespace x;

#define __NAMESPACE_ABE                 abe
#define __BEGIN_NAMESPACE_ABE           __BEGIN_NAMESPACE(__NAMESPACE_ABE)
#define __END_NAMESPACE_ABE             __END_NAMESPACE(__NAMESPACE_ABE)

#define __NAMESPACE_PRIVATE             _privates
#define __BEGIN_NAMESPACE_ABE_PRIVATE   __BEGIN_NAMESPACE(__NAMESPACE_ABE) __BEGIN_NAMESPACE(__NAMESPACE_PRIVATE)
#define __END_NAMESPACE_ABE_PRIVATE     __END_NAMESPACE(__NAMESPACE_PRIVATE) __END_NAMESPACE(__NAMESPACE_ABE)

#define USING_NAMESPACE_ABE             __USING_NAMESPACE(__NAMESPACE_ABE)

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

//=============================================================
// sp Type
__BEGIN_NAMESPACE_ABE

// https://stackoverflow.com/questions/6271615/any-way-to-prevent-dynamic-allocation-of-a-class
/**
 * static object that can not be shared
 * @note usally for local auxiliary class
 */
#define STATIC_OBJECT(TypeName)             \
    private:                                \
    void *  operator new(size_t);           \
    void    operator delete(void *)
struct ABE_EXPORT StaticObject {
    StaticObject() { }
    ~StaticObject() { }
    STATIC_OBJECT(StaticObject);
};

// FIXME: limit T to basic types
template <typename T> class Atomic : public StaticObject {
    private:
        volatile T value;

    public:
        ABE_INLINE Atomic()                     { ABE_ATOMIC_STORE(&value, static_cast<T>(0));  }
        ABE_INLINE Atomic(const Atomic& rhs)    { ABE_ATOMIC_STORE(&value, rhs.load());         }
        ABE_INLINE Atomic(T _value)             { ABE_ATOMIC_STORE(&value, _value);             }

        ABE_INLINE void   store(T val)          { ABE_ATOMIC_STORE(&value, val);                }
        ABE_INLINE T      load() const          { return ABE_ATOMIC_LOAD(&value);               }
        ABE_INLINE T      exchange(T val)       { return ABE_ATOMIC_EXCHANGE(&value, val);      }
        ABE_INLINE Bool   cas(T& to, T val)     { return ABE_ATOMIC_CAS(&value, &to, val);      }   // compare and swap

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
        UInt32          mID;
        Atomic<UInt>    mRefs;

    protected:
        SharedObject(const uint32_t id = FOURCC('?obj'));
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
        ABE_INLINE UInt32   GetObjectID() const { return mID; }

        /**
         * get this object reference count
         * @return return current reference count
         */
        ABE_INLINE UInt32   GetRetainCount() const { return mRefs.load(); }

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
        UInt32              ReleaseObject(Bool keep = False);

        /**
         * is this object shared with others
         * @note if object is not shared, it is safe to do anything
         *       else either copy this object or lock it to modify its context
         */
        ABE_INLINE Bool    IsObjectShared() const      { return GetRetainCount() > 1; }
        ABE_INLINE Bool    IsObjectNotShared() const   { return !IsObjectShared(); }

        DISALLOW_EVILS(SharedObject);
};

#define COMPARE(_op_)                                                   \
    ABE_INLINE Bool operator _op_ (const sp<T>& o) const {              \
        return mShared _op_ o.mShared;                                  \
    }                                                                   \
    ABE_INLINE Bool operator _op_ (const T* o) const {                  \
        return mShared _op_ o;                                          \
    }                                                                   \
    template<typename U>                                                \
    ABE_INLINE Bool operator _op_ (const sp<U>& o) const {              \
        return mShared _op_ o.mShared;                                  \
    }                                                                   \
    template<typename U>                                                \
    ABE_INLINE Bool operator _op_ (const U* o) const {                  \
        return mShared _op_ o;                                          \
    }                                                                   \

/**
 * Shared/Strong Pointer for SharedObject only
 */
template <class T> class ABE_EXPORT sp : public StaticObject {
    public:
        // constructors
        ABE_INLINE sp() : mShared(Nil) { }
        ABE_INLINE sp(SharedObject * rhs)                       { set(rhs);             }
        ABE_INLINE sp(const sp<T>& rhs)                         { set(rhs.mShared);     }
        template<typename U> ABE_INLINE sp(U * rhs)             { set(rhs);             }
        template<typename U> ABE_INLINE sp(const sp<U>& rhs)    { set(rhs.mShared);     }

        // destructors
        ABE_INLINE ~sp() { clear(); }

        // copy assignments
        ABE_INLINE sp& operator=(SharedObject * rhs)                    { clear(); set(rhs); return *this;          }
        ABE_INLINE sp& operator=(const sp<T>& rhs)                      { clear(); set(rhs.mShared); return *this;  }
        template<typename U> ABE_INLINE sp& operator=(U * rhs)          { clear(); set(rhs); return *this;          }
        template<typename U> ABE_INLINE sp& operator=(const sp<U>& rhs) { clear(); set(rhs.mShared); return *this;  }

        // clear
        ABE_INLINE void clear();

        // Nil test
        ABE_INLINE Bool isNil() const { return mShared == Nil ; }

    public:
        /**
         * @note compare the object pointer, not the object content
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
        // DEBUGGING
        ABE_INLINE UInt32      refsCount() const  { return mShared->GetRetainCount();         }

    public:
        template<typename U> friend class sp;
        ABE_INLINE void set(SharedObject *);
        template<typename U> ABE_INLINE void set(U *);
        SharedObject *  mShared;
};
#undef COMPARE

///////////////////////////////////////////////////////////////////////////
template <typename T> void sp<T>::set(SharedObject * shared) {
    mShared = shared;
    if (mShared) { mShared->RetainObject(); }
}

template<typename T> template<typename U> void sp<T>::set(U * shared) {
    mShared = static_cast<SharedObject *>(shared);
    if (mShared) { mShared->RetainObject(); }
}

template<typename T> void sp<T>::clear() {
    if (mShared) {
        // we should clear mShared before ReleaseObject() to avoid loop
        // in object destruction. but it will also make this object
        // inaccessable in onLastRetain()
        mShared->ReleaseObject();
        mShared = Nil;
    }
}

__END_NAMESPACE_ABE
#endif // __cplusplus

#endif // ABE_HEADERS_TYPES_H

