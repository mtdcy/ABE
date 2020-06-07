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

#define LOG_TAG "Bits"
//#define LOG_NDEBUG    0
#include "ABE/core/Log.h"
#include "Bits.h"

#define _bitmask(n)     ((1u << (n)) - 1)
#define _bitmask64(n)   ((1ull << (n)) - 1)

#include <string.h> // strlen

__BEGIN_NAMESPACE_ABE

///////////////////////////////////////////////////////////////////////////
BitReader::BitReader(const char *data, size_t length) :
    mData(data), mLength(length),
    mHead(0), mReservoir(0), mBitsLeft(0), mByteOrder(Little)
{
    CHECK_NULL(mData);
}

void BitReader::reset() const {
    mHead = 0;
    mReservoir = 0;
    mBitsLeft = 0;
}

size_t BitReader::remains() const {
    return 8 * (mLength - mHead) + mBitsLeft;
}

size_t BitReader::offset() const {
    return mHead * 8 - mBitsLeft;
}

void BitReader::skip(size_t n) const {
    DEBUG("skip %zu", n);
    CHECK_GT(n, 0);
    CHECK_GE(remains(), n);

    if (n <= mBitsLeft) {
        DEBUG("mReservoir = %#x, mBitsLeft %zu", mReservoir, mBitsLeft);
        mBitsLeft -= n;
        mReservoir &= _bitmask64(mBitsLeft);
        DEBUG("mReservoir = %#x, mBitsLeft %zu", mReservoir, mBitsLeft);
    } else {
        n -= mBitsLeft;
        mBitsLeft = 0;
        mReservoir = 0;

        mHead += n / 8;

        n %= 8;
        if (n) {
            mReservoir = (uint32_t)mData[mHead++];
            mBitsLeft  = 8 - n;
            mReservoir &= _bitmask64(mBitsLeft);
        }
        DEBUG("mReservoir = %#x, mBitsLeft %zu", mReservoir, mBitsLeft);
    }
}

uint32_t BitReader::show(size_t n) const {
    CHECK_GT(n, 0);
    CHECK_LE(n, 32);
    CHECK_GE(remains(), n);

    if (n > mBitsLeft) {
        DEBUG("request %zu bits, but %zu left", n, mBitsLeft);
        size_t m        = n - mBitsLeft;
        size_t numBytes = (m + 7) / 8;  // round up.

        if (numBytes + mHead > mLength)
            numBytes = mLength - mHead;

        DEBUG("read %zu bytes", numBytes);
        DEBUG("mReservoir = %#x", mReservoir);
        for (size_t i = 0; i < numBytes; i++) {
            mReservoir = (mReservoir << 8) | (uint8_t)mData[mHead++];
            mBitsLeft  += 8;
        }
        DEBUG("mReservoir = %#x, mBitsLeft %zu", mReservoir, mBitsLeft);
    }
    CHECK_LE(n, mBitsLeft);

    const uint32_t v = (mReservoir >> (mBitsLeft - n)) & _bitmask64(n);

    DEBUG("v = %#x", v);
    return v;
}

uint32_t BitReader::read(size_t n) const {
    uint32_t v = show(n);
    mReservoir &= _bitmask64(mBitsLeft - n);
    mBitsLeft -= n;
    DEBUG("mReservoir = %#x, mBitsLeft %zu", mReservoir, mBitsLeft);
    return v;
}

String BitReader::readS(size_t n) const {
    CHECK_LE(n * 8, remains());

    String s;

    if (__builtin_expect(mBitsLeft == 0, true)) {
        s = String(mData + mHead, n);
        mHead += n;
    } else {
        uint8_t tmp[n];
        for (size_t i = 0; i < n; i++) tmp[i] = r8();
        s = String((char*)&tmp[0], n);
    }
    return s;
}

sp<Buffer> BitReader::readB(size_t n) const {
    CHECK_LE(n * 8, remains());

    if (__builtin_expect(mBitsLeft == 0, true)) {
        sp<Buffer> b = new Buffer(n);
        b->write(mData + mHead, n);
        mHead += n;
        return b;
    } else {
        sp<Buffer> buffer = new Buffer(n);
        char * data = buffer->data();
        for (size_t i = 0; i < n; i++) *(data + i) = r8();
        buffer->step(n);
        return buffer;
    }
}

uint8_t BitReader::r8() const {
    CHECK_GE(remains(), 8);
    return read(8);
}

uint16_t BitReader::rl16() const {
    CHECK_GE(remains(), 16);

    uint16_t v = r8();
    v = v | r8() << 8;
    return v;
}

uint32_t BitReader::rl24() const {
    CHECK_GE(remains(), 24);

    uint32_t v = r8();
    v = v | (uint32_t)rl16() << 8;
    return v;
}

uint32_t BitReader::rl32() const {
    CHECK_GE(remains(), 32);

    uint32_t v = rl16();
    v = v | (uint32_t)rl16() << 16;
    return v;
}

uint64_t BitReader::rl64() const {
    CHECK_GE(remains(), 64);

    uint64_t v = rl32();
    v = v | (uint64_t)rl32() << 32;
    return v;
}

uint16_t BitReader::rb16() const {
    CHECK_GE(remains(), 16);

    uint16_t v = r8();
    v = r8() | v << 8;
    return v;
}

uint32_t BitReader::rb24() const {
    CHECK_GE(remains(), 24);

    uint32_t v = rb16();
    v = r8() | v << 8;
    return v;
}

uint32_t BitReader::rb32() const {
    CHECK_GE(remains(), 32);

    uint32_t v = rb16();
    v = rb16() | v << 16;
    return v;
}

uint64_t BitReader::rb64() const {
    CHECK_GE(remains(), 64);

    uint64_t v = rb32();
    v = rb32() | v << 32;
    return v;
}


///////////////////////////////////////////////////////////////////////////
BitWriter::BitWriter(char *data, size_t n) : mData(data), mSize(n),
    mHead(0), mReservoir(0), mBitsPopulated(0)
{
    CHECK_NULL(mData);
}

BitWriter::BitWriter(Buffer& data) : mData(data.data()), mSize(data.capacity()),
    mHead(0), mReservoir(0), mBitsPopulated(0)
{
    CHECK_NULL(mData);
}

size_t BitWriter::numBitsLeft() const {
    return 8 * (mSize - mHead) - mBitsPopulated;
}

void BitWriter::reset() {
    mHead = 0;
    mReservoir = 0;
    mBitsPopulated = 0;
}

void BitWriter::write(uint32_t x, size_t n) {
    CHECK_LE(n, 32);
    CHECK_GE(numBitsLeft(), n);

    mReservoir = (mReservoir << n) | x;
    mBitsPopulated  += n;

    while (mBitsPopulated >= 8) {
        mBitsPopulated -= 8;
        mData[mHead++] = (mReservoir >> mBitsPopulated) & 0xff;
        mReservoir &= _bitmask64(mBitsPopulated);
    }
}

void BitWriter::write() {
    if (mBitsPopulated == 0) return;

    CHECK_LT(mBitsPopulated, 8);

    write(0, 8-mBitsPopulated);

    CHECK_EQ(mBitsPopulated, 0);
}

void BitWriter::writeS(const String& s, size_t n) {
    if (!n || n > s.size()) n = s.size();
    CHECK_GE(numBitsLeft(), n * 8);

    for (size_t i = 0; i < n; i++) w8(s[i]);
}

void BitWriter::writeB(const Buffer& b, size_t n) {
    if (!n || n > b.ready()) n = b.ready();
    CHECK_GE(numBitsLeft(), n * 8);

    for (size_t i = 0; i < n; i++) w8(b[i]);
}

void BitWriter::w8(uint8_t x) {
    CHECK_GE(numBitsLeft(), 8);

    if (__builtin_expect(mBitsPopulated == 0, true)) {
        mData[mHead++] = x;
    } else {
        write(x, 8);
    }
}

void BitWriter::wl16(uint16_t x) {
    CHECK_GE(numBitsLeft(), 16);

    w8(x & 0xff);
    w8(x >> 8);
}

void BitWriter::wl24(uint32_t x) {
    CHECK_GE(numBitsLeft(), 24);

    w8(x & 0xff);
    wl16(x >> 8);
}

void BitWriter::wl32(uint32_t x) {
    CHECK_GE(numBitsLeft(), 32);

    wl16(x & 0xffff);
    wl16(x >> 16);
}

void BitWriter::wl64(uint64_t x) {
    CHECK_GE(numBitsLeft(), 64);

    wl32(x & 0xffffffff);
    wl32(x >> 32);
}

void BitWriter::wb16(uint16_t x) {
    CHECK_GE(numBitsLeft(), 16);

    w8(x >> 8);
    w8(x & 0xff);
}

void BitWriter::wb24(uint32_t x) {
    CHECK_GE(numBitsLeft(), 24);

    wb16(x >> 8);
    w8(x & 0xff);
}

void BitWriter::wb32(uint32_t x) {
    CHECK_GE(numBitsLeft(), 32);

    wb16(x >> 16);
    wb16(x & 0xffff);
}

void BitWriter::wb64(uint64_t x) {
    CHECK_GE(numBitsLeft(), 64);

    wb32(x >> 32);
    wb32(x & 0xffffffff);
}

__END_NAMESPACE_ABE
