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
#include <ABE/basic/Allocator.h>

#ifndef _TOOLKIT_HEADERS_BUFFER_H
#define _TOOLKIT_HEADERS_BUFFER_H 

///////////////////////////////////////////////////////////////////////////////
// Memory Management Layer [C++ Implementation]
//
#include <ABE/basic/String.h>
#include <ABE/basic/SharedObject.h>
#include <ABE/stl/List.h>
#include <ABE/tools/Mutex.h>

__BEGIN_DECLS
enum eBufferFlags {
    // TODO
    BUFFER_READ         = 0x1,  ///< implicit flag
    BUFFER_WRITE        = 0x2,  ///< this buffer can be modified
    BUFFER_RESIZABLE    = 0x4,  ///< this buffer can be resize
    BUFFER_RING         = 0x10, ///< implement ring buffer, take twice memory as it need
    BUFFER_SHARED       = 0x20, ///< copy-on-write
    BUFFER_READONLY     = BUFFER_READ,
    BUFFER_READWRITE    = BUFFER_READ | BUFFER_WRITE,
    BUFFER_DEFAULT      = BUFFER_READWRITE | BUFFER_RESIZABLE,
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
class Buffer : public SharedObject {
    public:
        /**
         * alloc an empty buffer with given capacity
         * @param capacity  initial capacity of this buffer
         * @param flags     flags of this buffer, @see eBufferFlags
         */
        Buffer(size_t capacity, const sp<Allocator>& allocator = kAllocatorDefault);
        Buffer(size_t capacity, eBufferFlags flags, const sp<Allocator>& allocator = kAllocatorDefault);

        /**
         * alloc a buffer by duplicate a null-terminated string
         * @param data  pointer hold the string
         * @param n     length to duplicate
         * @param flags flags of this buffer, @see eBufferFlags
         * @note if BUFFER_READONLY set, on memory will be allocate -> TODO
         */
        Buffer(const char *, size_t n = 0, eBufferFlags flags = BUFFER_DEFAULT,
                const sp<Allocator>& allocator = kAllocatorDefault);

        /**
         * copy constructor and assignment
         * the new buffer keeps the old buffer's flags & capacity & content
         */
        Buffer(const Buffer&);
        Buffer& operator=(const Buffer&);

        ~Buffer();

        /**
         * alloc a readonly buffer with preallocated memory, without duplicate
         * @param data  pointer to the memory
         * @param size  size of the memory
         * @param flags flags of this buffer, @see eBufferFlags
         * @return return reference to a readonly buffer
         */
        static sp<Buffer>   FromData(const char *data, size_t size);

        //! DEBUGGING
        String      string(bool hex = false) const;

    public:
        /**
         * get flags of this buffer
         * @return return the flags @see eBufferFlags
         */
        __ALWAYS_INLINE eBufferFlags flags() const { return mFlags; }

        /**
         * get the backend memory capacity
         * @return return the capacity in bytes
         */
        __ALWAYS_INLINE size_t capacity() const { return mCapacity; }

        /**
         * reset read & write position of this buffer
         */
        __ALWAYS_INLINE void reset() { mReadPos = mWritePos = 0; }

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

        __ALWAYS_INLINE size_t write(const Buffer& s, size_t n = 0)                 { return write(s.data(), n);                            }
        __ALWAYS_INLINE void replace(size_t offset, const Buffer& s, size_t n = 0)  { return replace(offset, s.data(), n ? n : s.size());   }

    public:
        // how many bytes avaible for read
        __ALWAYS_INLINE size_t ready() const    { return mWritePos - mReadPos;  }
        __ALWAYS_INLINE size_t size() const     { return ready();               } // alias
        size_t          read(char *buf, size_t n);
        sp<Buffer>      read(size_t n);
        sp<Buffer>      split(size_t pos, size_t size) const;

        int             compare(size_t offset, const char *s, size_t n = 0) const;
        __ALWAYS_INLINE int compare(const char *s, size_t n = 0) const                      { return compare(0, s, n);                              }
        __ALWAYS_INLINE int compare(const Buffer& s, size_t n = 0) const                    { return compare(0, s.data(), n);                       }
        __ALWAYS_INLINE int compare(size_t offset, const Buffer& s, size_t n = 0) const     { return compare(offset, s.data(), n ? n : s.size());   }

        ssize_t         indexOf(size_t offset, const char *s, size_t n = 0) const;
        __ALWAYS_INLINE ssize_t indexOf(const char *s, size_t n = 0) const                  { return indexOf(0, s, n);                              }
        __ALWAYS_INLINE ssize_t indexOf(const Buffer& s, size_t n = 0) const                { return indexOf(0, s.data(), n ? n : s.size());        }
        __ALWAYS_INLINE ssize_t indexOf(size_t offset, const Buffer& s, size_t n = 0) const { return indexOf(offset, s.data(), n ? n : s.size());   }

        __ALWAYS_INLINE bool operator==(const char *s) const    { return compare(s) == 0; }
        __ALWAYS_INLINE bool operator==(const Buffer& s) const  { return compare(s) == 0; }
        __ALWAYS_INLINE bool operator!=(const char *s) const    { return compare(s) != 0; }
        __ALWAYS_INLINE bool operator!=(const Buffer& s) const  { return compare(s) != 0; }

    public:
        // move read pointer forward
        void            skip(size_t n);
        // move write pointer forward
        void            step(size_t n);

    public:
        __ALWAYS_INLINE char*       data()          { return mData + mReadPos; }
        __ALWAYS_INLINE const char* data() const    { return mData + mReadPos; }
        __ALWAYS_INLINE char&       operator[](size_t index)        { return *(data() + index); }
        __ALWAYS_INLINE const char& operator[](size_t index) const  { return *(data() + index); }
        __ALWAYS_INLINE const char& at(size_t index) const          { return operator[](index); }

    private:
        void            _rewind();
        void            _alloc();
        void            _edit();

    private:
        Buffer();

        // backend memory provider
        SharedBuffer *      mSharedBuffer;
        // context
        sp<Allocator>       mAllocator;
        char *              mData;
        size_t              mCapacity;
        eBufferFlags        mFlags;
        size_t              mReadPos;
        size_t              mWritePos;
};

__END_NAMESPACE_ABE

#endif // __cplusplus 

#endif // _TOOLKIT_HEADERS_BUFFER_H 


