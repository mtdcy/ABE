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
Buffer::Buffer(size_t capacity, const sp<Allocator>& allocator) :
    mSharedBuffer(NULL),
    mAllocator(allocator),
    mData(NULL), mCapacity(capacity),
    mFlags(BUFFER_DEFAULT), mReadPos(0), mWritePos(0)
{
    CHECK_GT(mCapacity, 0);
    _alloc();
}

Buffer::Buffer(size_t capacity, eBufferFlags flags, const sp<Allocator>& allocator) :
    mSharedBuffer(NULL),
    mAllocator(allocator),
    mData(NULL), mCapacity(capacity),
    mFlags(flags), mReadPos(0), mWritePos(0)
{
    CHECK_GT(mCapacity, 0);
    _alloc();
}

Buffer::Buffer(const char *s, size_t n, eBufferFlags flags, const sp<Allocator>& allocator) :
    mSharedBuffer(NULL),
    mAllocator(allocator),
    mData(NULL), mCapacity(n ? n : strlen(s)),
    mFlags(flags), mReadPos(0), mWritePos(0)
{
    CHECK_GT(mCapacity, 0);
    _alloc();
    memcpy(mData, s, n);
    mWritePos += n;
}

void Buffer::_alloc() {
    size_t allocLength = mCapacity;
    if (mFlags & BUFFER_RING) allocLength <<= 1;

    if (mFlags & BUFFER_SHARED) {
        mSharedBuffer = SharedBuffer::Create(mAllocator, allocLength);
        mData = mSharedBuffer->data();
    } else {
        mData = (char *)mAllocator->allocate(allocLength);
    }
    CHECK_NULL(mData);
}

Buffer::Buffer(const Buffer& rhs) :
    mSharedBuffer(NULL),
    mAllocator(rhs.mAllocator),
    mData(NULL), mCapacity(rhs.mCapacity),
    mFlags(rhs.mFlags), mReadPos(0), mWritePos(0)
{
    if (mFlags & BUFFER_SHARED) {
        mSharedBuffer = rhs.mSharedBuffer->RetainBuffer();
        mData = mSharedBuffer->data();
        // no need to copy content
    } else {
        _alloc();
        memcpy(mData, rhs.data(), rhs.ready());
    }

    mWritePos += rhs.ready();
}

Buffer& Buffer::operator=(const Buffer& rhs) {
    DEBUG("copy operator");
    // release old
    if (mFlags & BUFFER_SHARED) {
        mSharedBuffer->ReleaseBuffer();
        mSharedBuffer = NULL;
    } else {
        mAllocator->deallocate(mData);
    }
    mData = NULL;
    mReadPos = mWritePos = 0;
    // no change to allocator

    // copy new
    mCapacity = rhs.mCapacity;
    mFlags = rhs.mFlags;
    if (mFlags & BUFFER_SHARED) {
        mSharedBuffer = rhs.mSharedBuffer->RetainBuffer();
        mData = mSharedBuffer->data();
    } else {
        _alloc();
        memcpy(mData, rhs.data(), rhs.ready());
    }
    return *this;
}

Buffer::~Buffer() {
    if (mFlags & BUFFER_SHARED) {
        mSharedBuffer->ReleaseBuffer();
        mSharedBuffer = NULL;
    } else {
        mAllocator->deallocate(mData);
    }
}

Buffer::Buffer() :
    mSharedBuffer(NULL),
    mAllocator(kAllocatorDefault),
    mData(NULL), mCapacity(0),
    mFlags(BUFFER_READONLY), mReadPos(0), mWritePos(0)
{
}

sp<Buffer> Buffer::FromData(const char *data, size_t size) {
    //CHECK_FALSE(flags & BUFFER_SHARED);     // can NOT be shared.
    //CHECK_FALSE(flags & BUFFER_RESIZABLE);  // can NOT be resizable

    sp<Buffer> buffer = new Buffer;
    buffer->mData = const_cast<char*>(data);
    buffer->mCapacity = size;
    buffer->mFlags = BUFFER_READONLY;
    return buffer;
}

status_t Buffer::resize(size_t cap) {
    if (!(mFlags & BUFFER_WRITE)) return PERMISSION_DENIED;
    if (!(mFlags & BUFFER_RESIZABLE)) return INVALID_OPERATION;

    size_t allocLength = cap;
    if (mFlags & BUFFER_RING) allocLength <<= 1;

    if (mFlags & BUFFER_SHARED) {
        mSharedBuffer = mSharedBuffer->edit(allocLength);
        mData = mSharedBuffer->data();
    } else {
        mData = (char *)mAllocator->reallocate(mData, allocLength);
    }
    mCapacity = cap;
    return OK;
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
    if (mFlags & BUFFER_RING) {
        return capacity() - ready();
    }
    return capacity() - mWritePos;
}

size_t Buffer::write(const char *s, size_t n) {
    CHECK_TRUE(mFlags & BUFFER_WRITE);
    if (!n)     n = strlen(s);
    CHECK_LE(n, empty());

    _edit();

    memcpy(mData + mWritePos, s, n);
    mWritePos   += n;

    _rewind();
    return ready();
}

size_t Buffer::write(int c, size_t n) {
    CHECK_TRUE(mFlags & BUFFER_WRITE);
    CHECK_GT(n, 0);
    CHECK_LE(n, empty());

    _edit();

    memset(mData + mWritePos, c, n);
    mWritePos   += n;

    _rewind();
    return ready();
}

void Buffer::replace(size_t pos, int c, size_t n) {
    CHECK_TRUE(mFlags & BUFFER_WRITE);
    CHECK_GT(n, 0);
    CHECK_LT(pos, ready());
    CHECK_LE(pos + n, ready() + empty());

    _edit();

    memset(mData + mReadPos + pos, c, n);
}

void Buffer::replace(size_t pos, const char *s, size_t n) {
    CHECK_TRUE(mFlags & BUFFER_WRITE);
    CHECK_LT(pos, ready());
    CHECK_LE(pos + n, ready() + empty());

    _edit();

    if (!n)     n = strlen(s);
    memcpy(mData + mReadPos + pos, s, n);
}

void Buffer::step(size_t n) {
    CHECK_TRUE(mFlags & BUFFER_WRITE);
    CHECK_GT(n, 0);
    CHECK_LE(n, empty());

    _edit();

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

sp<Buffer> Buffer::read(size_t n) {
    CHECK_GT(n, 0);
    if (ready() == 0) return NULL;
    n = MIN(n, ready());
    sp<Buffer> buf = new Buffer(data(), n);
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

sp<Buffer> Buffer::split(size_t pos, size_t n) const {
    CHECK_GT(n, 0);
    CHECK_LE(pos + n, ready());
    sp<Buffer> buf = new Buffer(n);
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

void Buffer::_edit() {
    CHECK_TRUE(mFlags & BUFFER_WRITE);

    if (mFlags & BUFFER_SHARED) {
        mSharedBuffer = mSharedBuffer->edit();
        mData = mSharedBuffer->data();
    }
}

// have to avoid too much rewind ops
void Buffer::_rewind() {
    if ((mFlags & BUFFER_RING) && (mReadPos >= capacity())) {
        memmove(mData, data(), ready());
        mWritePos   -= mReadPos;
        mReadPos    = 0;
    }
}

__END_NAMESPACE_ABE
