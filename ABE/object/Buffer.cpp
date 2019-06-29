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


// File:    Buffer.cpp
// Author:  mtdcy.chen
// Changes: 
//          1. 20160701     initial version
//

#define LOG_TAG   "Buffer"
//#define LOG_NDEBUG 0
#include "ABE/basic/Log.h"
#include "Buffer.h"

#include <string.h> // strlen
#include <ctype.h>  // isprint

#define MIN(a, b)   (a) > (b) ? (b) : (a)

__BEGIN_NAMESPACE_ABE

static String hexdump(const void *data, uint32_t bytes) {
    String result;
    const uint8_t *head = (const uint8_t*)data;
    const uint8_t *end = head + bytes;

    while (head < end) {
        String line = String::format("%08" PRIx32 ": ", (uint32_t)(head - (uint8_t*)data));

        for (int i = 0; i < 16; i++) {
            if (head + i < end)
                line.append(String::format("%02" PRIx8 " ", head[i]));
            else
                line.append("   ");
        }

        line.append("> ");

        for (int i = 0; i < 16; i++) {
            if (head + i < end) {
                if (isprint(head[i]))
                    line.append(String::format("%c", head[i]));
                else
                    line.append(".");
            } else {
                line.append(" ");
            }
        }
        head += 16;

        line.append("\n");
        result.append(line);
    }

    return result;
}

///////////////////////////////////////////////////////////////////////////
Buffer::Buffer(size_t capacity, const Object<Allocator>& allocator) : SharedObject(OBJECT_ID_BUFFER),
    mAllocator(allocator),
    mData(NULL), mCapacity(capacity),
    mType(kBufferTypeDefault), mReadPos(0), mWritePos(0)
{
    CHECK_GT(mCapacity, 0);
    _alloc();
}

Buffer::Buffer(size_t capacity, eBufferType type, const Object<Allocator>& allocator) : SharedObject(OBJECT_ID_BUFFER),
    mAllocator(allocator),
    mData(NULL), mCapacity(capacity),
    mType(type), mReadPos(0), mWritePos(0)
{
    CHECK_GT(mCapacity, 0);
    _alloc();
}

Buffer::Buffer(const char *s, size_t n, eBufferType type, const Object<Allocator>& allocator) : SharedObject(OBJECT_ID_BUFFER),
    mAllocator(allocator),
    mData(NULL), mCapacity(n ? n : strlen(s)),
    mType(type), mReadPos(0), mWritePos(0)
{
    CHECK_GT(mCapacity, 0);
    _alloc();
    memcpy(mData, s, n);
    mWritePos += n;
}

void Buffer::_alloc() {
    size_t allocLength = mCapacity;
    if (mType == kBufferTypeRing) allocLength <<= 1;
    mData = (char *)mAllocator->allocate(allocLength);
    CHECK_NULL(mData);
}

Buffer::~Buffer() {
    mAllocator->deallocate(mData);
}

int Buffer::resize(size_t cap) {
    size_t allocLength = cap;
    if (mType == kBufferTypeRing) allocLength <<= 1;
    mData = (char *)mAllocator->reallocate(mData, allocLength);
    mCapacity = cap;
    return 0;
}

String Buffer::string(bool hex) const {
    String result = String::format("Buffer %zu@%p [%zu, %zu]\n", 
            capacity(), mData, mReadPos, mWritePos);

    if (hex) {
        result.append(hexdump(data(), ready() > 128 ? 128 : ready()));
    }
    return result;
}

size_t Buffer::empty() const {
    if (mType == kBufferTypeRing) {
        return capacity() - ready();
    }
    return capacity() - mWritePos;
}

size_t Buffer::write(const char *s, size_t n) {
    if (!n)     n = strlen(s);
    CHECK_LE(n, empty());

    memcpy(mData + mWritePos, s, n);
    mWritePos   += n;

    _rewind();
    return ready();
}

size_t Buffer::write(int c, size_t n) {
    CHECK_GT(n, 0);
    CHECK_LE(n, empty());

    memset(mData + mWritePos, c, n);
    mWritePos   += n;

    _rewind();
    return ready();
}

void Buffer::replace(size_t pos, int c, size_t n) {
    CHECK_GT(n, 0);
    CHECK_LT(pos, ready());
    CHECK_LE(pos + n, ready() + empty());

    memset(mData + mReadPos + pos, c, n);
}

void Buffer::replace(size_t pos, const char *s, size_t n) {
    CHECK_LT(pos, ready());
    CHECK_LE(pos + n, ready() + empty());

    if (!n)     n = strlen(s);
    memcpy(mData + mReadPos + pos, s, n);
}

void Buffer::step(size_t n) {
    CHECK_GT(n, 0);
    CHECK_LE(n, empty());

    mWritePos   += n;

    _rewind();
}

size_t Buffer::read(char *buf, size_t n) {
    CHECK_GT(n, 0);
    if (ready() == 0) return 0;

    n = MIN(n, ready());

    memcpy(buf, data(), n);
    mReadPos    += n;

    _rewind();
    return n;
}

Object<Buffer> Buffer::read(size_t n) {
    CHECK_GT(n, 0);
    if (ready() == 0) return NULL;
    n = MIN(n, ready());
    Object<Buffer> buf = new Buffer(data(), n);
    mReadPos += n;
    _rewind();
    return buf;
}

void Buffer::skip(size_t n) {
    CHECK_GT(n, 0);
    CHECK_LE(n, ready());

    mReadPos    += n;

    _rewind();
}

Object<Buffer> Buffer::split(size_t pos, size_t n) const {
    CHECK_GT(n, 0);
    CHECK_LE(pos + n, ready());
    Object<Buffer> buf = new Buffer(n);
    buf->write(data() + pos, n);
    return buf;
}

int Buffer::compare(size_t offset, const char *s, size_t n) const {
    if (!n)     n = strlen(s);
    CHECK_LE(n, ready());
    return memcmp(data() + offset, s, n);
}

ssize_t Buffer::indexOf(size_t offset, const char *s, size_t n) const {
    if (!n)     n = strlen(s);

    char *s1    = (char*)memchr(data(), s[0], ready());
    while (s1) {
        if (!memcmp(s1, s, n)) return s1 - data();

        ++s1;
        s1 = (char*)memchr(s1, s[0], data() + ready() - s1);
    }

    return -1;
}

// have to avoid too much rewind ops
void Buffer::_rewind() {
    if ((mType == kBufferTypeRing) && (mReadPos >= capacity())) {
        memmove(mData, data(), ready());
        mWritePos   -= mReadPos;
        mReadPos    = 0;
    }
}

__END_NAMESPACE_ABE
