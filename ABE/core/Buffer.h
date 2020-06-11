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

#include <ABE/core/Types.h>

#ifndef ABE_HEADERS_BUFFER_H
#define ABE_HEADERS_BUFFER_H 

///////////////////////////////////////////////////////////////////////////////
// Memory Management Layer [C++ Implementation]
#include <ABE/core/Allocator.h>
#include <ABE/core/String.h>

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
class ABE_EXPORT Buffer : public SharedObject {
    public:
        enum eBufferType {
            Linear,             ///< linear buffer
            Ring,               ///< implement ring buffer, take twice memory as it need
            Default = Linear
        };
    
        /**
         * alloc an empty buffer with given capacity
         * @param capacity  initial capacity of this buffer
         * @param type      type of this buffer, @see eBufferType
         */
        Buffer(size_t capacity, const sp<Allocator>& allocator = kAllocatorDefault);
        Buffer(size_t capacity, eBufferType type, const sp<Allocator>& allocator = kAllocatorDefault);

        /**
         * alloc a buffer by duplicate a null-terminated string
         * @param data  pointer hold the string
         * @param n     length to duplicate
         * @param type  type of this buffer, @see eBufferType
         */
        Buffer(const char *, size_t n = 0, eBufferType type = Default,
                const sp<Allocator>& allocator = kAllocatorDefault);

        ~Buffer();

        //! DEBUGGING
        String      string(bool hex = false) const;

    public:
    public:
        ABE_INLINE char*       data()                           { return mData + mReadPos;  }
        ABE_INLINE const char* data() const                     { return mData + mReadPos;  }
        ABE_INLINE char&       operator[](size_t index)         { return *(data() + index); }
        ABE_INLINE const char& operator[](size_t index) const   { return *(data() + index); }
        ABE_INLINE const char& at(size_t index) const           { return operator[](index); }

        /**
         * get flags of this buffer
         * @return return the flags @see eBufferFlags
         */
        ABE_INLINE eBufferType type() const { return mType; }

        /**
         * get the backend memory capacity
         * @return return the capacity in bytes
         */
        ABE_INLINE size_t       capacity() const { return mCapacity; }

        /**
         * reset read & write position of this buffer
         */
        ABE_INLINE void         reset() { mReadPos = mWritePos = 0; }

        /**
         * resize this buffer's backend memory
         * @param cap   new capacity of the backend memory
         * @return return true on success
         */
        bool                    resize(size_t cap);

    public:
        // how many bytes avaible for write
        size_t              empty() const;
    
        // write bytes into buffer
        size_t              write(const char *s, size_t n = 0);
        size_t              write(int c, size_t n);
        ABE_INLINE size_t   write(const Buffer& s, size_t n = 0) { return write(s.data(), n ? n : s.size()); }

        // move write pointer forward
        void                step(size_t n);
    
    public:
        // how many bytes avaible for read
        ABE_INLINE size_t   ready() const { return mWritePos - mReadPos; }
        ABE_INLINE size_t   size() const { return ready(); } ///<  alias for ready()
    
        size_t              read(char *buf, size_t n) const;
        sp<Buffer>          read(size_t n) const;

        // move read pointer forward
        void                skip(size_t n) const;

    public:
        /**
         * replace bytes in buffer 
         * @param offset pos related to read pos, offset must smaller than ready()
         * @param s pointer to a string
         * @param n how may bytes to replace, n must smaller than capacity()
         * @note if n = 0, s must be a null-terminated string and 
         *                  the whole string will be used to replace
         */
        void                replace(size_t offset, const char *s, size_t n = 0);
        void                replace(size_t offset, int c, size_t n);

        ABE_INLINE void     replace(const char *s, size_t n = 0) { replace(0, s, n); }
        ABE_INLINE void     replace(int c, size_t n = 0) { replace(0, c, n); }
        ABE_INLINE void     replace(size_t offset, const Buffer& s, size_t n = 0) { return replace(offset, s.data(), n ? n : s.size()); }
        ABE_INLINE void     replace(const Buffer& s, size_t n = 0) { replace(0, s, n); }

    public:
        sp<Buffer>          split(size_t pos, size_t size) const;

    public:
        int                 compare(size_t offset, const char *s, size_t n = 0) const;

        ABE_INLINE int      compare(const char *s, size_t n = 0) const { return compare(0, s, n); }
        ABE_INLINE int      compare(size_t offset, const Buffer& s, size_t n = 0) const { return compare(offset, s.data(), n ? n : s.size()); }
        ABE_INLINE int      compare(const Buffer& s, size_t n = 0) const { return compare(0, s, n); }

    public:
        ABE_INLINE bool     operator==(const char *s) const    { return compare(s) == 0; }
        ABE_INLINE bool     operator==(const Buffer& s) const  { return compare(s) == 0; }
        ABE_INLINE bool     operator!=(const char *s) const    { return compare(s) != 0; }
        ABE_INLINE bool     operator!=(const Buffer& s) const  { return compare(s) != 0; }

    public:
        ssize_t             indexOf(size_t offset, const char *s, size_t n = 0) const;

        ABE_INLINE ssize_t  indexOf(const char *s, size_t n = 0) const { return indexOf(0, s, n); }
        ABE_INLINE ssize_t  indexOf(size_t offset, const Buffer& s, size_t n = 0) const { return indexOf(offset, s.data(), n ? n : s.size());   }
        ABE_INLINE ssize_t  indexOf(const Buffer& s, size_t n = 0) const { return indexOf(0, s, n); }

    private:
        void                _rewind() const;
        void                _alloc();

    private:
        // backend memory provider
        sp<Allocator>       mAllocator;
        mutable char *      mData;
        size_t              mCapacity;
        const eBufferType   mType;
        mutable size_t      mReadPos;
        mutable size_t      mWritePos;
    
    DISALLOW_EVILS(Buffer);
};

__END_NAMESPACE_ABE

#endif // ABE_HEADERS_BUFFER_H 


