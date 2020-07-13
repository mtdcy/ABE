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
#include "ABE/core/Log.h"
#include "Buffer.h"

#include <string.h> // strlen
#include <ctype.h>  // isprint

#define MIN(a, b)   (a) > (b) ? (b) : (a)
#define MASK64(n)   ((1ull << (n)) - 1)

__BEGIN_NAMESPACE_ABE

static String hexdump(const void *data, UInt32 bytes) {
    String result;
    const UInt8 *head = (const UInt8*)data;
    const UInt8 *end = head + bytes;

    while (head < end) {
        String line = String::format("%08" PRIx32 ": ", (UInt32)(head - (UInt8*)data));

        for (Int i = 0; i < 16; i++) {
            if (head + i < end)
                line.append(String::format("%02" PRIx8 " ", head[i]));
            else
                line.append("   ");
        }

        line.append("> ");

        for (Int i = 0; i < 16; i++) {
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

UInt32 ABuffer::show(UInt32 n) const {
    CHECK_GT(n, 0);
    CHECK_LE(n, 32);

    if (n > mReadReservoir.mLength) {
        DEBUG("request %zu bits, but %zu left", n, mReadReservoir.mLength);
        const UInt32 m  = n - mReadReservoir.mLength;
        UInt32 numBytes = (m + 7) / 8;  // round up.
        CHECK_LE((Int64)numBytes, size());

        DEBUG("read %zu bytes", numBytes);
        DEBUG("mReservoir = %#x", mReadReservoir.mBits);
        for (UInt32 i = 0; i < numBytes; i++) {
            mReadReservoir.mBits = (mReadReservoir.mBits << 8) | readByte();
            mReadReservoir.mLength  += 8;
        }
        DEBUG("mReservoir = 0x%#x, mBitsLeft %zu", mReadReservoir.mBits,
              mReadReservoir.mLength);
    }
    CHECK_LE(n, mReadReservoir.mLength);
    
    const UInt32 v = (mReadReservoir.mBits >> (mReadReservoir.mLength - n)) & MASK64(n);
    DEBUG("v = %#x", v);
    return v;
}

UInt32 ABuffer::read(UInt32 n) const {
    UInt32 x = show(n);
    mReadReservoir.mLength -= n;
    return x;
}

void ABuffer::skip(UInt32 n) const {
    if (n < mReadReservoir.mLength) {
        mReadReservoir.mLength -= n;
        return;
    }
    n -= mReadReservoir.mLength;
    mReadReservoir.mLength = 0;
    if (n >= 8) {
        skipBytes(n / 8);
        n = n % 8;
    }
    if (n > 0) read(n);
}

void ABuffer::skip() const {
    mReadReservoir.mLength = 0;
}

void ABuffer::write(UInt32 x, UInt32 n) {
    CHECK_LE(n, 32);

    mWriteReservoir.mBits = (mWriteReservoir.mBits << n) | (UInt64)x;
    mWriteReservoir.mLength += n;

    while (mWriteReservoir.mLength >= 8) {
        mWriteReservoir.mLength -= 8;
        writeByte((mWriteReservoir.mBits >> mWriteReservoir.mLength) & 0xff);
    }
}

void ABuffer::write() {
    if (mWriteReservoir.mLength == 0) return;
    write(0, 8 - mWriteReservoir.mLength);
    CHECK_EQ(mWriteReservoir.mLength, 0);
}

UInt8 ABuffer::r8() const {
    return read(8);
}

UInt16 ABuffer::rl16() const {
    UInt16 v = r8();
    v = v | r8() << 8;
    return v;
}

UInt32 ABuffer::rl24() const {
    UInt32 v = r8();
    v = v | (UInt32)rl16() << 8;
    return v;
}

UInt32 ABuffer::rl32() const {
    UInt32 v = rl16();
    v = v | (UInt32)rl16() << 16;
    return v;
}

UInt64 ABuffer::rl64() const {
    UInt64 v = rl32();
    v = v | (UInt64)rl32() << 32;
    return v;
}

UInt16 ABuffer::rb16() const {
    UInt16 v = r8();
    v = r8() | v << 8;
    return v;
}

UInt32 ABuffer::rb24() const {
    UInt32 v = rb16();
    v = r8() | v << 8;
    return v;
}

UInt32 ABuffer::rb32() const {
    UInt32 v = rb16();
    v = rb16() | v << 16;
    return v;
}

UInt64 ABuffer::rb64() const {
    UInt64 v = rb32();
    v = rb32() | v << 32;
    return v;
}

String ABuffer::rs(UInt32 n) const {
    UInt8 s[n];
    for (UInt32 i = 0; i < n; i++) s[i] = r8();
    return String((Char*)&s[0], n);
}

void ABuffer::w8(UInt8 x) {
    write(x, 8);
}

void ABuffer::wl16(UInt16 x) {
    w8(x & 0xff);
    w8(x >> 8);
}

void ABuffer::wl24(UInt32 x) {
    w8(x & 0xff);
    wl16(x >> 8);
}

void ABuffer::wl32(UInt32 x) {
    wl16(x & 0xffff);
    wl16(x >> 16);
}

void ABuffer::wl64(UInt64 x) {
    wl32(x & 0xffffffff);
    wl32(x >> 32);
}

void ABuffer::wb16(UInt16 x) {
    w8(x >> 8);
    w8(x & 0xff);
}

void ABuffer::wb24(UInt32 x) {
    wb16(x >> 8);
    w8(x & 0xff);
}

void ABuffer::wb32(UInt32 x) {
    wb16(x >> 16);
    wb16(x & 0xffff);
}

void ABuffer::wb64(UInt64 x) {
    wb32(x >> 32);
    wb32(x & 0xffffffff);
}

void ABuffer::ws(const String& s, UInt32 n) {
    if (!n || n > s.size()) n = s.size();
    for (UInt32 i = 0; i < n; i++) w8(s[i]);
}

void ABuffer::reset() const {
    mReadReservoir.mLength;
}

void ABuffer::flush() {
    write();
}

///////////////////////////////////////////////////////////////////////////
Buffer::Buffer(const Buffer * rhs, UInt32 offset, UInt32 size) : ABuffer(),
    mAllocator(rhs->mAllocator), mData(rhs->mData->RetainBuffer()),
    mOffset(rhs->mOffset + offset), mCapacity(size),
    mType(Linear), mReadPos(0), mWritePos(size) {
        CHECK_GT(mCapacity, 0);
}

Buffer::Buffer(UInt32 capacity, const sp<Allocator>& allocator) : ABuffer(),
    mAllocator(allocator), mData(Nil),
    mOffset(0), mCapacity(capacity),
    mType(Linear), mReadPos(0), mWritePos(0)
{
    CHECK_GT(mCapacity, 0);
    mData = alloc();
}

Buffer::Buffer(UInt32 capacity, eBufferType type, const sp<Allocator>& allocator) : ABuffer(),
    mAllocator(allocator), mData(Nil),
    mOffset(0), mCapacity(capacity),
    mType(type), mReadPos(0), mWritePos(0)
{
    CHECK_GT(mCapacity, 0);
    mData = alloc();
}

Buffer::Buffer(const Char *s, UInt32 n, eBufferType type, const sp<Allocator>& allocator) : ABuffer(),
    mAllocator(allocator), mData(Nil),
    mOffset(0), mCapacity(n ? n : strlen(s)),
    mType(type), mReadPos(0), mWritePos(0)
{
    CHECK_GT(mCapacity, 0);
    mData = alloc();
    memcpy(mData->data(), s, n);
    mWritePos += n;
}

void Buffer::onFirstRetain() { }

void Buffer::onLastRetain() {
    mData->ReleaseBuffer();
}

String Buffer::string(Bool hex) const {
    String result = String::format("Buffer %zu@%p [%zu, %zu]\n", 
            capacity(), mData, mReadPos, mWritePos);

    if (hex) {
        result.append(hexdump(data(), size() > 128 ? 128 : size()));
    }
    return result;
}

Int64 Buffer::empty() const {
    if (mType == Ring) {
        return capacity() - size();
    }
    return capacity() - mWritePos;
}

Int64 Buffer::offset() const {
    if (mType == Ring) {
        if (mWritePos > capacity())
            return capacity() - size();
        else
            return mReadPos;
    }
    return mReadPos;
}

void Buffer::resetBytes() const {
    ABuffer::reset();
    mReadPos -= offset();
}

void Buffer::flushBytes() {
    edit();
    rewind(1);
    ABuffer::flush();
}

void Buffer::clearBytes() {
    edit();
    mReadPos = mWritePos = 0;
}

UInt32 Buffer::writeBytes(const Char * s, UInt32 n) {
    if (!n) n = strlen((const Char *)s);
    CHECK_GT(n, 0);
    edit();
    rewind(n);
    ABuffer::flush();
    const UInt32 m = MIN(n, empty());
    memcpy(mData->data() + mOffset + mWritePos, s, m);
    mWritePos += m;
    return m;
}

UInt32 Buffer::writeBytes(const sp<ABuffer>& buf, UInt32 n) {
    if (!n) n = buf->size();
    CHECK_GT(n, 0);
    edit();
    rewind(n);
    ABuffer::flush();
    const UInt32 m = MIN(n, empty());
    buf->readBytes(mData->data() + mOffset + mWritePos, m);
    mWritePos += m;
    return m;
}

UInt32 Buffer::writeBytes(Int c, UInt32 n) {
    CHECK_GT(n, 0);
    edit();
    rewind(n);
    ABuffer::flush();
    const UInt32 m = MIN(n, empty());
    memset(mData->data() + mOffset + mWritePos, c, m);
    mWritePos += m;
    return m;
}

Char * Buffer::base() {
    ABuffer::reset();
    edit();
    rewind(empty());
    ABuffer::flush();
    return mData->data() + mOffset;
}

UInt32 Buffer::readBytes(Char * buf, UInt32 n) const {
    CHECK_GT(n, 0);
    ABuffer::reset();
    if (size() == 0) return 0;
    n = MIN(n, size());
    memcpy(buf, data(), n);
    mReadPos    += n;
    return n;
}

UInt32 Buffer::stepBytes(UInt32 n) {
    CHECK_GT(n, 0);
    // NO edit() & rewind() here
    // NO ABuffer::flush() here.
    const UInt32 m = MIN(n, size() + empty());
    // NO memset here
    mWritePos = mReadPos + m;
    return size();
}

void Buffer::setBytesRange(UInt32 offset, UInt32 n) {
    CHECK_LT(offset, capacity());
    CHECK_GT(n, 0);
    // NO edit() & rewind() here
    const UInt32 m = MIN(n, capacity() - offset);
    // NO memset here
    mReadPos    = offset;
    mWritePos   = mReadPos + n;
}

sp<ABuffer> Buffer::readBytes(UInt32 n) const {
    CHECK_GT(n, 0);
    ABuffer::reset();
    if (size() == 0) return Nil;
    n = MIN(n, size());
    sp<Buffer> data = new Buffer(this, mReadPos, n);
    mReadPos += n;
    return data;
}

Int64 Buffer::skipBytes(Int64 n) const {
    CHECK_GE(n, -offset());
    CHECK_LE(n, size());
    ABuffer::reset();
    mReadPos    += n;
    return offset();
}

UInt8 Buffer::readByte() const {
    CHECK_GE(size(), 1);
    UInt8 x = (UInt8)*data();
    ++mReadPos;
    return x;
}

sp<ABuffer> Buffer::cloneBytes() const {
    return new Buffer(this, 0, size());
}

void Buffer::writeByte(UInt8 x) {
    edit();
    rewind(1);
    CHECK_GE(empty(), 1);
    *(mData->data() + mOffset + mWritePos) = x;
    ++mWritePos;
}

// alloc a shared buffer
SharedBuffer * Buffer::alloc() {
    UInt32 allocLength = mCapacity;
    if (mType == Ring) allocLength <<= 1;
    SharedBuffer * data = SharedBuffer::Create(mAllocator, allocLength);
    CHECK_NULL(data);
    return data;
}

void Buffer::edit() {
    if (mData->IsBufferNotShared()) return;
    SharedBuffer * data = alloc();
    // copy all data to new buffer
    memcpy(data->data(), mData->data() + mOffset, mWritePos);
    mData->ReleaseBuffer();
    mData = data;
    mOffset = 0;
    CHECK_NULL(mData);
}

Bool Buffer::resize(UInt32 cap) {
    mCapacity = cap;
    if (mData->IsBufferNotShared() && mOffset == 0) {
        // resize using SharedBuffer::edit()
        UInt32 allocLength = mCapacity;
        if (mType == Ring) allocLength <<= 1;
        mData = mData->edit(allocLength);
    } else {
        // allocate a new SharedBuffer
        SharedBuffer * data = alloc();
        memcpy(data->data(), mData->data() + mOffset, mWritePos);
        mData->ReleaseBuffer();
        mData = data;
        mOffset = 0;
    }
    
    if (mReadPos > mCapacity) mReadPos = mCapacity;
    if (mWritePos > mCapacity) mWritePos = mCapacity;
    
    return mData != Nil;
}

// have to avoid too much rewind ops
void Buffer::rewind(UInt32 n) {
    // for Linear buffer, rewind do NOTHING
    if (mType != Ring) return;

    // avoid rewind too much
    // rewind only when no enough space for writeBytes
    if (mWritePos + n < mCapacity * 2) return;
    
    // rewind always keep capacity() bytes or size() bytes?
    // 1. if we keep capacity() bytes, readBytes can always
    //  access capacity() bytes data after rewind, but rewind
    //  will take place more often.
    // 2. if we keep size() bytes, readBytes can only access
    //  size() bytes after rewind, and rewind also take place
    //  at the crucial moment.
    // we will take option 2
    memmove(mData->data() + mOffset,
            mData->data() + mOffset + mReadPos,
            size());
    mWritePos   -= mReadPos;
    mReadPos    = 0;
}

__END_NAMESPACE_ABE
