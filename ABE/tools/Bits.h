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


// File:    Bits.cpp
// Author:  mtdcy.chen
// Changes: 
//          1. 20160701     initial version
//

#ifndef _TOOLKIT_HEADERS_BITS_H
#define _TOOLKIT_HEADERS_BITS_H

#include <ABE/object/Buffer.h>
#ifdef __cplusplus
__BEGIN_NAMESPACE_ABE
/**
 * read bits from data
 * @note always do in-place operation
 */
class BitReader {
    public:
        /**
         * create a bit reader based on data.
         * @param data  reference to buffer holding data
         */
        BitReader(const char *data, size_t n);
        BitReader(const Buffer& data);
        ~BitReader();

    public:
        size_t      size() const { return mLength; }

        /**
         * get how many bits left for read
         * @return return total bits left in the data.
         */
        size_t      numBitsLeft() const;

        /**
         * reset context of bit reader, so we can read from
         * the begin again.
         */
        void        reset();

        /**
         * get current read offset
         * @return return current bit offset
         */
        size_t      offset() const;

    public:
        /**
         * read 32 bits at most from data and step
         * @param nbits     number bits to read
         * @return return the value
         * @note read start from low memory to high.
         *       no byte-order consider.
         */
        uint32_t    read(size_t nbits) const;

        /**
         * show 32 bits at most of data without step
         * @param nbits     number bits to show
         * @return return the value
         */
        uint32_t    show(size_t nbits) const;

        /**
         * read a string from data and step
         * @param nbytes    number bytes to read
         * @return return a string contains the data
         */
        String      readS(size_t nbytes) const;

        /**
         * read a block from data and step
         * @param nbytes    number bytes to read
         * @return return a buffer reference contains the data
         */
        sp<Buffer>  readB(size_t nbytes) const;

        /**
         * skip n bits at most of the data
         * @param nbits     number bits to skip
         */
        void        skip(size_t nbits) const;

        /**
         * skip current byte trailing.
         */
        __ALWAYS_INLINE void skip() const { skip(mBitsLeft); }

        /**
         * skip n bytes of the data
         */
        __ALWAYS_INLINE void skipBytes(size_t nbytes) const { skip(nbytes << 3); }

        /**
         * read 8 bits from data
         * @return return 8 bits as uint8_t
         */
        uint8_t     r8() const;

        /**
         * read 16 bits as little endian from data
         * @return return 16 bits as uint16_t
         */
        uint16_t    rl16() const;

        /**
         * read 24 bits as little endian from data
         * @return return 24 bits as uint32_t
         */
        uint32_t    rl24() const;

        /**
         * read 32 bits as little endian from data
         * @return return 32 bits as uint32_t
         */
        uint32_t    rl32() const;

        /**
         * read 64 bits as little endian from data
         * @return return 64 bits as uint64_t
         */
        uint64_t    rl64() const;

        /**
         * read 16 bits as big endian from data
         * @return return 16 bits as uint16_t
         */
        uint16_t    rb16() const;

        /**
         * read 24 bits as big endian from data
         * @return return 24 bits as uint32_t
         */
        uint32_t    rb24() const;

        /**
         * read 32 bits as big endian from data
         * @return return 32 bits as uint32_t
         */
        uint32_t    rb32() const;

        /**
         * read 64 bits as big endian from data
         * @return return 64 bits as uint64_t
         */
        uint64_t    rb64() const;

    private:
        const char *        mData;
        size_t              mLength;
        mutable size_t      mHead;
        mutable uint64_t    mReservoir;
        mutable size_t      mBitsLeft;

    private:
        // no need of copy
        DISALLOW_EVILS(BitReader);
};

class BitWriter {
    public:
        BitWriter(char *data, size_t n);

        BitWriter(Buffer& data);

        ~BitWriter();

    public:
        __ALWAYS_INLINE const char * data() const { return mData; }

        __ALWAYS_INLINE size_t       size() const { return mHead + (mBitsPopulated + 7) / 8; }

    public:
        size_t      numBitsLeft() const;

        void        reset();

    public:
        /**
         * write n bits to buffer
         * @param x     holds the value to write
         * @param nbits accept the lowest n bits of x
         */
        void        write(uint32_t x, size_t nbits);

        /**
         * write a string to buffer
         * @param s     string to write
         * @param n     write the first n bytes of s, if n == 0,
         *              write whole string, exclude terminating null byte
         */
        void        writeS(const String& s, size_t n = 0);

        /**
         * write a block to buffer
         * @param b     buffer hold the block of data.
         * @param n     write the first n bytes of data, if n == 0,
         *              write the whole block.
         */
        void        writeB(const Buffer& b, size_t limit = 0);

        /**
         * write to byte boundary
         * @note always write() before you access data()/buffer()
         */
        void        write();

        /**
         * write 8 bits to buffer
         * @param x     holds the 8 bits value
         */
        void        w8(uint8_t x);

        /**
         * write 16 bits as little endian to buffer
         * @param x     holds the 16 bits value
         */
        void        wl16(uint16_t x);

        /**
         * write 24 bits as little endian to buffer
         * @param x     holds the 24 bits value
         */
        void        wl24(uint32_t x);

        /**
         * write 32 bits as little endian to buffer
         * @param x     holds the 32 bits value
         */
        void        wl32(uint32_t x);

        /**
         * write 64 bits as little endian to buffer
         * @param x     holds the 64 bits value
         */
        void        wl64(uint64_t x);

        /**
         * write 16 bits as big endian to buffer
         * @param x     holds the 16 bits value
         */
        void        wb16(uint16_t x);

        /**
         * write 24 bits as big endian to buffer
         * @param x     holds the 24 bits value
         */
        void        wb24(uint32_t x);

        /**
         * write 32 bits as big endian to buffer
         * @param x     holds the 32 bits value
         */
        void        wb32(uint32_t x);

        /**
         * write 64 bits as big endian to buffer
         * @param x     holds the 64 bits value
         */
        void        wb64(uint64_t x);

    private:
        char *      mData;
        size_t      mSize;
        size_t      mHead;
        uint64_t    mReservoir;
        size_t      mBitsPopulated;

    private:
        DISALLOW_EVILS(BitWriter);
};
__END_NAMESPACE_ABE
#endif // __cplusplus

#endif // _TOOLKIT_HEADERS_BITS_H 
