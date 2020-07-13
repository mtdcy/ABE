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


// File:    Content.h
// Author:  mtdcy.chen
// Changes: 
//          1. 20160701     initial version
//

#define LOG_TAG "Content"
//#define LOG_NDEBUG 0
#include "Log.h"
#include "Content.h"

#define MIN(a, b)   ((a) > (b) ? (b) : (a))

__BEGIN_NAMESPACE_ABE

// prepare cache block before reading data
void Content::prepareBlock(UInt32 n) const {
    if (mReadBlock->size() >= n) return;
    
    // cache in underrun, fetch kBlockLength bytes
    if (n > mReadBlock->empty() || mReadBlock->empty() < mProto->blockLength()) {
        const UInt32 times = (n + mReadBlock->size()) / mReadBlock->capacity() + 1;
        // resize may fail, but DO NOT assert here, let readBytes handle it
        if (mReadBlock->resize(mReadBlock->capacity() * times) == False) {
            ERROR("Block resize failed %zu -> %zu",
                  (UInt32)mReadBlock->capacity(),
                  (UInt32)mReadBlock->capacity() * times);
        }
    }
    DEBUG("prepare %zu bytes with block capacity %zu(%zu) bytes @ %" PRId64,
          n, (UInt32)mReadBlock->capacity(), (UInt32)mReadBlock->size(), offset());
    
    UInt32 read = mProto->readBytes(mReadBlock);
    mReadPosition += read;
    DEBUG("prepare read %zu bytes, current block size %zu, current @ %" PRId64,
          read, mReadBlock->size(), offset());
}

void Content::writeBlockBack(Bool force) {
    if (mWriteBlock->size() == 0) return;
    
    // write block when it is full
    if (!force && mWriteBlock->empty() > 0) return;
    
    const UInt32 size = mWriteBlock->size();
    UInt32 n = mProto->writeBytes(mWriteBlock);
    if (n == 0) {
        ERROR("No more space ?");
        return;
    }
    
    // does protocol skip bytes??
    if (mWriteBlock->size() == size) {
        mWriteBlock->skipBytes(n);
    }
    mWritePosition += n;
}

///////////////////////////////////////////////////////////////////////////
// static
sp<Protocol> CreateFile(const String& url, Protocol::eMode mode);

sp<Content> Content::Create(const String& url, Protocol::eMode mode) {
    INFO("Open content %s", url.c_str());

    sp<Protocol> proto;
    if (url.startsWith("file://", True) ||
            url.startsWith("android://", True) ||
            url.startsWith("pipe://", True)) {
        proto = CreateFile(url, mode);
    } else {
        // normal file path
        proto = CreateFile(url, mode);
    }

    if (proto.isNil()) {
        ERROR("unsupported url %s", url.c_str());
        return Nil;
    }

    return Content::Create(proto);
}

sp<Content> Content::Create(const sp<Protocol>& proto) {
    return new Content(proto, proto->blockLength());
}

// read mode: read - read - read
// write mode: write - write - write
// read & write mode: read - write - write - ... - writeBack
Content::Content(const sp<Protocol>& proto, UInt32 blockLength) :
    ABuffer(), mProto(proto),
    mReadPosition(0), mReadBlock(Nil),
    mWritePosition(0), mWriteBlock(Nil) {
    if (mProto->mode() & Protocol::Read) {
        mReadBlock = new Buffer(blockLength, Buffer::Ring);
    }
    if (mProto->mode() & Protocol::Write) {
        mWriteBlock = new Buffer(blockLength, Buffer::Ring);
    }
}

void Content::onFirstRetain() {
    
}

void Content::onLastRetain() {
    if (!mWriteBlock.isNil()) writeBlockBack(True);
}

Int64 Content::capacity() const {
    return mProto->totalBytes();
}

Int64 Content::size() const {
    if (capacity() <= 0) {
        prepareBlock(1);
        return mReadBlock->size();
    }
    return capacity() - offset();
}

Int64 Content::empty() const {
    if (capacity() <= 0) {
        // THIS IS TRUE
        // for stream media, file size maybe unknown
        return 0;
    }
    return capacity() - size();
}

Int64 Content::offset() const {
    return mReadPosition - mReadBlock->size();
}

const Char * Content::data() const {
    ABuffer::reset();
    prepareBlock(1);
    return mReadBlock->data();
}

sp<ABuffer> Content::readBytes(UInt32 n) const {
    DEBUG("read %zu bytes @ %" PRId64, n, offset());
    ABuffer::reset();
    prepareBlock(n);
    if (mReadBlock->size() == 0) {
        INFO("End Of File");
        return Nil;
    }
    sp<Buffer> data = mReadBlock->readBytes(n);
    DEBUG("read return %zu bytes, end @ %" PRId64, data->size(), offset());
    return data;
}

UInt32 Content::readBytes(Char * buffer, UInt32 n) const {
    ABuffer::reset();
    prepareBlock(n);
    if (mReadBlock->size() == 0) {
        INFO("End Of File");
        return Nil;
    }
    return mReadBlock->readBytes(buffer, n);
}

Int64 Content::skipBytes(Int64 delta) const {
    DEBUG("skip %" PRId64 " bytes @ %" PRId64,
          delta, offset());
    ABuffer::reset();
    if (delta < 0) {
        if (-delta < mReadBlock->offset()) {
            mReadBlock->skipBytes(delta);
        } else {
            mReadPosition = mProto->seekBytes(offset() + delta);
            mReadBlock->clearBytes();
        }
    } else {
        if (delta < mReadBlock->size()) {
            mReadBlock->skipBytes(delta);
        } else {
            mReadPosition = mProto->seekBytes(offset() + delta);
            mReadBlock->clearBytes();
        }
    }
    DEBUG("current pos @ %" PRId64, offset());
    return offset();
}

sp<ABuffer> Content::cloneBytes() const {
    ERROR("TODO: clone bytes in content bytes");
    return Nil;
}

UInt32 Content::writeBytes(const Char * data, UInt32 n) {
    ABuffer::flush();
    UInt32 bytesWritten = 0;
    while (bytesWritten < n) {
        writeBlockBack();
        UInt32 m = mWriteBlock->writeBytes(data, n - bytesWritten);
        if (m == 0) break;
        data += m;
        bytesWritten += m;
    }
    return bytesWritten;
}

UInt32 Content::writeBytes(const sp<ABuffer>& buffer, UInt32 n) {
    ABuffer::flush();
    while (buffer->size()) {
        writeBlockBack();
        UInt32 m = mWriteBlock->writeBytes(buffer);
        if (m == 0) break;
        buffer->skipBytes(m);
    }
    return n - buffer->size();
}

UInt32 Content::writeBytes(int c, UInt32 n) {
    ABuffer::flush();
    UInt32 bytesWritten = 0;
    while (bytesWritten < n) {
        writeBlockBack();
        UInt32 m = mWriteBlock->writeBytes(c, n - bytesWritten);
        if (m == 0) break;
        bytesWritten += m;
    }
    return bytesWritten;
}

void Content::resetBytes() const {
    DEBUG("resetBytes @ %" PRId64, offset());
    ABuffer::reset();
    if (offset() < mReadBlock->offset()) {
        // this is the first block, reset block only
        mReadBlock->resetBytes();
        DEBUG("current @ %" PRId64, offset());
        return;
    }
    
    // drop all data in block
    mReadBlock->clearBytes();
    // seek protocal here ?
    // we work as a FIFO buffer, seek here will cause underlying
    // protocol seek too much when client read & reset frequently.
    mReadPosition = mProto->seekBytes(0);
    DEBUG("current @ %" PRId64, offset());
}

void Content::flushBytes() {
    ABuffer::flush();
    writeBlockBack(True);
}

void Content::clearBytes() {
    // TODO
}

UInt8 Content::readByte() const {
    prepareBlock(1);
    return mReadBlock->r8();
}

void Content::writeByte(UInt8 x) {
    if (mWriteBlock->empty() == 0)
        writeBlockBack();
    mWriteBlock->w8(x);
}

__END_NAMESPACE_ABE

