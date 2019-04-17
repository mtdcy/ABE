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


// File:    Buffer.h
// Author:  mtdcy.chen
// Changes: 
//          1. 20160701     initial version
//

#include <ABE/basic/Types.h>

#ifndef _TOOLKIT_HEADERS_BUFFER_H
#define _TOOLKIT_HEADERS_BUFFER_H 

///////////////////////////////////////////////////////////////////////////////
// Memory Management Layer [C++ Implementation]
#include <ABE/basic/Allocator.h>
#include <ABE/basic/String.h>
#include <ABE/basic/SharedObject.h>

__BEGIN_DECLS
enum eBufferType {
    kBufferTypeDefault, ///< default buffer
    kBufferTypeRing,    ///< implement ring buffer, take twice memory as it need
};
__END_DECLS

#ifdef __cplusplus

__BEGIN_NAMESPACE_ABE
/**
 * Buffer:
 *                           write pos
 *                           v
 *  |-----------------------------------------------------------|
 *      ^
 *      read pos
 *
 * Buffer is not thread safe
 */
class __ABE_EXPORT Buffer : public SharedObject {
    public:
        /**
         * alloc an empty buffer with given capacity
         * @param capacity  initial capacity of this buffer
         * @param type      type of this buffer, @see eBufferType
         */
        Buffer(size_t capacity, const Object<Allocator>& allocator = kAllocatorDefault);
        Buffer(size_t capacity, eBufferType type, const Object<Allocator>& allocator = kAllocatorDefault);

        /**
         * alloc a buffer by duplicate a null-terminated string
         * @param data  pointer hold the string
         * @param n     length to duplicate
         * @param type  type of this buffer, @see eBufferType
         */
        Buffer(const char *, size_t n = 0, eBufferType type = kBufferTypeDefault,
                const Object<Allocator>& allocator = kAllocatorDefault);

        ~Buffer();

        //! DEBUGGING
        String      string(bool hex = false) const;

    public:
        /**
         * get flags of this buffer
         * @return return the flags @see eBufferFlags
         */
        __ABE_INLINE eBufferType type() const { return mType; }

        /**
         * get the backend memory capacity
         * @return return the capacity in bytes
         */
        __ABE_INLINE size_t capacity() const { return mCapacity; }

        /**
         * reset read & write position of this buffer
         */
        __ABE_INLINE void reset() { mReadPos = mWritePos = 0; }

        /**
         * resize this buffer's backend memory, BUFFER_RESIZABLE must be set
         * @param cap   new capacity of the backend memory
         * @return return OK on success, return PERMISSION_DENIED if readonly,
         *         return INVALID_OPERATION if not resizable.
         */
        status_t        resize(size_t cap);

    public:
        // how many bytes avaible for write
        size_t          empty() const;
        size_t          write(const char *s, size_t n = 0);
        size_t          write(int c, size_t n);
        void            replace(size_t offset, const char *s, size_t n = 0);
        void            replace(size_t offset, int c, size_t n);

        __ABE_INLINE size_t write(const Buffer& s, size_t n = 0)                 { return write(s.data(), n ? n : s.size());            }
        __ABE_INLINE void replace(size_t offset, const Buffer& s, size_t n = 0)  { return replace(offset, s.data(), n ? n : s.size());  }

    public:
        // how many bytes avaible for read
        __ABE_INLINE size_t ready() const       { return mWritePos - mReadPos;  }
        __ABE_INLINE size_t size() const        { return ready();               } // alias
    
        size_t          read(char *buf, size_t n);
        Object<Buffer>  read(size_t n);
    
        Object<Buffer>  split(size_t pos, size_t size) const;

        int             compare(size_t offset, const char *s, size_t n = 0) const;
        __ABE_INLINE int compare(const char *s, size_t n = 0) const                      { return compare(0, s, n);                             }
        __ABE_INLINE int compare(const Buffer& s, size_t n = 0) const                    { return compare(0, s.data(), n ? n : s.size());       }
        __ABE_INLINE int compare(size_t offset, const Buffer& s, size_t n = 0) const     { return compare(offset, s.data(), n ? n : s.size());  }

        ssize_t         indexOf(size_t offset, const char *s, size_t n = 0) const;
        __ABE_INLINE ssize_t indexOf(const char *s, size_t n = 0) const                  { return indexOf(0, s, n);                              }
        __ABE_INLINE ssize_t indexOf(const Buffer& s, size_t n = 0) const                { return indexOf(0, s.data(), n ? n : s.size());        }
        __ABE_INLINE ssize_t indexOf(size_t offset, const Buffer& s, size_t n = 0) const { return indexOf(offset, s.data(), n ? n : s.size());   }

        __ABE_INLINE bool operator==(const char *s) const    { return compare(s) == 0; }
        __ABE_INLINE bool operator==(const Buffer& s) const  { return compare(s) == 0; }
        __ABE_INLINE bool operator!=(const char *s) const    { return compare(s) != 0; }
        __ABE_INLINE bool operator!=(const Buffer& s) const  { return compare(s) != 0; }

    public:
        // move read pointer forward
        void            skip(size_t n);
        // move write pointer forward
        void            step(size_t n);

    public:
        __ABE_INLINE char*       data()          { return mData + mReadPos; }
        __ABE_INLINE const char* data() const    { return mData + mReadPos; }
        __ABE_INLINE char&       operator[](size_t index)        { return *(data() + index); }
        __ABE_INLINE const char& operator[](size_t index) const  { return *(data() + index); }
        __ABE_INLINE const char& at(size_t index) const          { return operator[](index); }

    private:
        void            _rewind();
        void            _alloc();

    private:
        DISALLOW_EVILS(Buffer);

        // backend memory provider
        Object<Allocator>   mAllocator;
        char *              mData;
        size_t              mCapacity;
        const eBufferType   mType;
        size_t              mReadPos;
        size_t              mWritePos;
};

__END_NAMESPACE_ABE

#endif // __cplusplus 

__BEGIN_DECLS
typedef SharedObjectRef     BufferRef;

__ABE_EXPORT BufferRef      BufferCreate(size_t);
#define BufferRelease(x)    BufferRelease((SharedObjectRef)x);

__ABE_EXPORT size_t         BufferCapacity(const BufferRef);
__ABE_EXPORT size_t         BufferSize(const BufferRef);

__END_DECLS

#endif // _TOOLKIT_HEADERS_BUFFER_H 


