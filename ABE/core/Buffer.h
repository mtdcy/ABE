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
 * base class for FIFO buffer
 * Buffer:
 *                           write pos
 *                           v
 *  |-----------------------------------------------------------|
 *      ^
 *      read pos
 *
 * ABuffer is not thread safe
 */
class ABE_EXPORT ABuffer : public SharedObject {
    public:
        enum eByteOrder { Little, Big };
        virtual ~ABuffer() { }
    
    public:
        /**
         * read buffer properties
         * @note abstract interface for inherited class
         */
        virtual int64_t     capacity() const = 0;   ///< return buffer capacity
        virtual int64_t     size() const = 0;       ///< return data size in bytes in buffer
        virtual int64_t     empty() const = 0;      ///< return empty bytes in buffer
        virtual int64_t     offset() const = 0;     ///< return current read position
    
    public:
        /**
         * read bytes from buffer
         * @note bytes operator will reset reader bits reservoir
         */
        virtual sp<ABuffer> readBytes(size_t) const = 0;        ///< read n bytes as a new buffer
        virtual size_t      readBytes(char *, size_t) const = 0;///< read n bytes into certain memory
        virtual int64_t     skipBytes(int64_t) const = 0;       ///< skip read n bytes, [-offset(), size()]
        virtual void        resetBytes() const = 0;             ///< reset bytes reader
        virtual sp<ABuffer> cloneBytes() const = 0;             ///< clone all bytes
        
        /**
         * write bytes into buffer
         * @note bytes writer will flush writer bits reservoir
         */
        virtual size_t      writeBytes(const char *, size_t n = 0) = 0;         ///< write n bytes into this buffer
        virtual size_t      writeBytes(const sp<ABuffer>& b, size_t n = 0) = 0; ///< write a buffer into this buffer
        virtual size_t      writeBytes(int c, size_t n) = 0;    ///< write n bytes of c
        virtual void        flushBytes() = 0;                   ///< flush bytes writer
        virtual void        clearBytes() = 0;                   ///< clear bytes in buffer

    protected:
        // basic operators for bit reader & writer
        virtual uint8_t     readByte() const = 0;               ///< read one byte
        virtual void        writeByte(uint8_t) = 0;             ///< write one byte

    public:
        /**
         * bit reader operators
         * @note client have to do sanity check on bits left for read
         */
        void                skip(size_t) const;         ///< skip n bits
        void                skip() const;               ///< skip current byte tail bits
        uint32_t            read(size_t) const;         ///< read n bits
        uint32_t            show(size_t) const;         ///< show n bits
        uint8_t             r8() const;                 ///< read 8 bits
        uint16_t            rl16() const;               ///< read 16 bits as little endian
        uint16_t            rb16() const;               ///< read 16 bits as big endian
        uint32_t            rl24() const;               ///< read 24 bits as little endian
        uint32_t            rb24() const;               ///< read 24 bits as big endian
        uint32_t            rl32() const;               ///< read 32 bits as little endian
        uint32_t            rb32() const;               ///< read 32 bits as big endian
        uint64_t            rl64() const;               ///< read 64 bits as little endian
        uint64_t            rb64() const;               ///< read 64 bits as big endian
        String              rs(size_t) const;           ///< read n char as string

        /**
         * bit writer operators
         * @note client have to do sanity check on bits left for write
         */
        void                write(uint32_t, size_t);    ///< write value x as n bits
        void                write();                    ///< write bits 0 to byte boundary
        void                w8(uint8_t);                ///< write 8 bits
        void                wl16(uint16_t);             ///< write 16 bits as little endian
        void                wb16(uint16_t);             ///< write 16 bits as big endian
        void                wl24(uint32_t);             ///< write 24 bits as little endian
        void                wb24(uint32_t);             ///< write 24 bits as big endian
        void                wl32(uint32_t);             ///< write 32 bits as little endian
        void                wb32(uint32_t);             ///< write 32 bits as big endian
        void                wl64(uint64_t);             ///< write 64 bits as little endian
        void                wb64(uint64_t);             ///< write 64 bits as big endian
        void                ws(const String&, size_t n = 0);    ///< write n byte string

        /**
         * set default byte order and read based on byte order
         * @note values in a structure always has the same byte order
         */
        ABE_INLINE eByteOrder byteOrder() const { return mByteOrder; }
        ABE_INLINE void     setByteOrder(eByteOrder order) const { mByteOrder = order; }
        ABE_INLINE uint16_t r16() const     { return mByteOrder == Big ? rb16() : rl16(); }
        ABE_INLINE uint32_t r24() const     { return mByteOrder == Big ? rb24() : rl24(); }
        ABE_INLINE uint32_t r32() const     { return mByteOrder == Big ? rb32() : rl32(); }
        ABE_INLINE uint64_t r64() const     { return mByteOrder == Big ? rb64() : rl64(); }
        ABE_INLINE void     w16(uint16_t x) { mByteOrder == Big ? wb16(x) : wl16(x); }
        ABE_INLINE void     w24(uint32_t x) { mByteOrder == Big ? wb24(x) : wl24(x); }
        ABE_INLINE void     w32(uint32_t x) { mByteOrder == Big ? wb32(x) : wl32(x); }
        ABE_INLINE void     w64(uint64_t x) { mByteOrder == Big ? wb64(x) : wl64(x); }

    protected:
        ABuffer() : mByteOrder(Little) {
            mReadReservoir.mLength = 0;
            mWriteReservoir.mLength = 0;
        }
        
        void    reset() const;  ///< reset reader reservoir
        void    flush();        ///< flush writer reservoir
    
    private:
        mutable eByteOrder mByteOrder;
        struct Reservoir {
            uint64_t    mBits;
            size_t      mLength;
        };
        mutable Reservoir mReadReservoir;
        Reservoir mWriteReservoir;
};

class ABE_EXPORT Buffer : public ABuffer {
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

        virtual ~Buffer();
    
        /**
         * get flags of this buffer
         * @return return the flags @see eBufferFlags
         */
        eBufferType type() const { return mType; }
    
        /**
         * resize this buffer's backend memory
         * @param cap   new capacity of the backend memory
         * @return return true on success
         */
        bool                resize(size_t cap);

        //! DEBUGGING
        String      string(bool hex = false) const;
    
    public:
        virtual int64_t     capacity() const { return mCapacity; }
        virtual int64_t     size() const { return mWritePos - mReadPos; }
        virtual int64_t     empty() const;
        virtual int64_t     offset() const;
    
    public:
        virtual sp<ABuffer> readBytes(size_t) const;
        virtual size_t      readBytes(char *, size_t) const;
        virtual int64_t     skipBytes(int64_t) const;
        virtual void        resetBytes() const;
        virtual sp<ABuffer> cloneBytes() const;
    
        virtual size_t      writeBytes(const char *, size_t n = 0);
        virtual size_t      writeBytes(const sp<ABuffer>&, size_t n = 0);
        virtual size_t      writeBytes(int c, size_t n);
        virtual void        flushBytes();
        virtual void        clearBytes();
    
    public:
        // access bytes directly. UNSAFE!
        // MUST avoid using these routines
        const char *        base() const { return mData->data() + mOffset; }
        const char *        data() const { return base() + mReadPos; }
        char *              base();
        char *              data() { return base() + mReadPos; }
        /**
         * move read pointer forword by n bytes
         * @note after write directly @ data()
         */
        size_t              stepBytes(size_t n);
        /**
         * set buffer range, read pos @ offset with n bytes data
         * @note after write directly @ base()
         */
        void                setBytesRange(size_t offset, size_t n);

    protected:
        virtual uint8_t     readByte() const;
        virtual void        writeByte(uint8_t);

    private:
        void                edit();
        void                rewind(size_t);
        SharedBuffer *      alloc();
    
    protected:
        // for creating cow buffer
        Buffer(const Buffer *, size_t, size_t);
    
        // backend memory provider
        sp<Allocator>       mAllocator;
        SharedBuffer *      mData;
        size_t              mOffset;    // for cow buffer
        size_t              mCapacity;
        const eBufferType   mType;
        mutable size_t      mReadPos;
        size_t              mWritePos;
    
        DISALLOW_EVILS(Buffer);
};

__END_NAMESPACE_ABE

#endif // ABE_HEADERS_BUFFER_H 


