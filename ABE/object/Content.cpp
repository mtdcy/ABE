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
#include "ABE/basic/Log.h"
#include "Content.h"

#include <string.h>  // memcpy

#include "protocol/File.h"
//#include "protocol/MemoryProto.h"

#define MIN(a, b)   ((a) > (b) ? (b) : (a))

__BEGIN_NAMESPACE_ABE

static size_t kMaxLineLength = 1024;

///////////////////////////////////////////////////////////////////////////
//! return false on error or eos
bool Content::readBlock() {
    mBlock->reset();
    mBlockOffset    = 0;
    mBlockLength    = 0;
    mBlockPopulated = false;

    // read cache first, and then read from cache.
    ssize_t ret = mProto->readBytes(
            mBlock->data(), 
            mBlock->empty());
    if (ret < 0) {
        // TODO: set error state
        ERROR("read: readBytes return error %d", ret);
        return false;
    } else if (ret == 0) {
        DEBUG("readBlock: EOS...");
        return false;
    }

    mBlock->step(ret);
    mBlockLength        = ret;

    mRangeOffset        += ret;

    // statistic
    mRealReadCnt++;
    mRealReadBytes += ret;

    return true;
}

//! return true on success
bool Content::writeBlockBack() {
    if (mBlockPopulated && mBlock->ready() > 0) {
        CHECK_TRUE(mProto->flags() & Protocol::WRITE);
        DEBUG("write back, real offset %" PRId64 ", cache offset %" PRId64, 
                mRangeOffset, mBlockOffset);

        DEBUG("write back: %s", mBlock->string().c_str());

        // in read & write mode, which read cache first, 
        // and then modify cache, then write back
        // so, we have to seek back before write back
        int64_t offset = mRangeOffset;
        const bool readMode = mProto->flags() & Protocol::READ;
        if (readMode && mBlockLength) {
            offset  -= mBlockLength;
            mProto->seekBytes(offset + mRangeStart);
        }

        ssize_t ret = mProto->writeBytes(mBlock->data(), mBlock->ready());

        if (ret < 0) { 
            ERROR("flush: writeBytes return error %d", ret);
            return false;
        } else if ((size_t)ret < mBlockOffset) { 
            // we have to make sure populated cache are been written back at least
            WARN("writeBytes: only %d of %d populated cache been written back", 
                    ret, mBlockOffset);
        }

        // update range offset & length
        mRangeOffset    = offset + ret;
        mRangeLength    = mProto->totalBytes() - mRangeStart;
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////
// static
sp<Content> Content::Create(const String& url, uint32_t mode) {
    INFO("Open content %s", url.c_str());

    sp<Protocol> proto;
    if (url.startsWithIgnoreCase("file://") || 
            url.startsWithIgnoreCase("/") ||
            url.startsWithIgnoreCase("android://") ||
            url.startsWithIgnoreCase("pipe://")) {
        proto = new content_protocol::File(url, mode);
#if 0
    } else if (url.startsWithIgnoreCase("buffer://")) {
        proto = new MemoryProto(url, mode);
#endif 
    } else {
        proto = new content_protocol::File(url, mode);
    }

    if (proto.get()) {
        sp<Content> pipe = new Content(proto);
        if (pipe->status() == 0) return pipe;
    }

    ERROR("unsupported url %s", url.c_str());
    return NULL;
}

// read mode: read - read - read
// write mode: write - write - write
// read & write mode: read - write - write - ... - writeBack
Content::Content(const sp<Protocol>& proto, size_t blockLength)
    :
        mProto(proto),
        mRangeStart(0),
        mRangeLength(proto->totalBytes()),
        mRangeOffset(0),
        mBlock(new Buffer(blockLength)), mBlockOffset(0), 
        mBlockLength(0), mBlockPopulated(false),
        mReadCnt(0), mRealReadCnt(0), mReadBytes(0), mRealReadBytes(0),
        mWriteCnt(0), mWriteBytes(0),
        mSeekCnt(0), mRealSeekCnt(0),
        mSkipCnt(0), mSkipSeekCnt(0)
{
}

Content::~Content() {
    flush();

    if (mProto->flags() & Protocol::READ) {
        INFO("cache heat %.2f(%d/%d), cache efficiency %.3f(%" PRId64 "/%" PRId64 ")", 
                (mReadCnt - mRealReadCnt) / (float)mReadCnt, 
                mReadCnt, mRealReadCnt,
                mReadBytes / (float)mRealReadBytes,
                mReadBytes, mRealReadBytes);
    }
}

uint32_t Content::flags() const {
    return mProto->flags();
}

status_t Content::status() const {
    if (mProto == 0) return UNKNOWN_ERROR;
    return mProto->status();
}

void Content::flush() {
    writeBlockBack();
    mBlock->reset();
    mBlockOffset = 0;
}

sp<Buffer> Content::read(size_t size) {
    CHECK_TRUE(mProto->flags() & Protocol::READ);

    sp<Buffer> data = new Buffer(size);

    bool eof = false;
    size_t n = size;

    while (n > 0) {
        int remains = mBlock->ready() - mBlockOffset;
        char *s = mBlock->data() + mBlockOffset;
        if (remains > 0) {
            size_t m = MIN(remains, n);
            //DEBUG("read %d bytes from cache.", m);

            data->write(s, m);

            mBlockOffset  += m;
            n                       -= m;
        } else {
            flush();

            // TODO: read directly from protocol 
            if (readBlock() == false) {
                eof = true;
                break;
            }
        } 
    }

    n               = size - n;

    mReadCnt       += 1;
    mReadBytes     += n;

    if (!n) return NULL;

    //DEBUG("%s", PRINTABLE(data->string()));

    return data;
}

sp<Buffer> Content::readLine() {
    CHECK_TRUE(mProto->flags() & Protocol::READ);

    sp<Buffer> line = new Buffer(kMaxLineLength);

    char *s = mBlock->data() + mBlockOffset;
    size_t m = mBlock->ready() - mBlockOffset; 

    bool eof = false;
    size_t i = 0;
    size_t j = 0;
    for (;;) {

        if (i == m || !m) {
            INFO("prepare next block");
            if (m) line->write(s, m);

            flush();

            if (readBlock() == false) {
                eof = true;
                break;
            }

            s = mBlock->data();
            m = mBlock->ready();
            i = 0;
        } 

        if (++j == kMaxLineLength) {
            // beyond max line range.
            INFO("beyond max line range.");
            if (i) line->write(s, i);

            // we suppose to use line->size() as this condition, but 
            // it is bad for performance
            CHECK_EQ(line->ready(), kMaxLineLength - 1);

            mBlockOffset += i;
            break;
        }

        if (s[i] == '\n') {
            INFO("find return byte at %zu", i);

            if (i) line->write(s, i);     

            mBlockOffset += i + 1;    // +1 to including the return byte

            break;
        }

        ++i;
    }

    // EOF
    if (eof && line->ready() == 0) return NULL;

    line->write('\0', 1);         // null-terminate 

    INFO("line: %s", line->data());

    return line;
}

ssize_t Content::write(const Buffer& data) {
    CHECK_TRUE(mProto->flags() & Protocol::WRITE);
    DEBUG("write: %s", PRINTABLE(data.string()));

    const bool readMode = mProto->flags() & Protocol::READ;

    const char *s = data.data();
    size_t n = data.ready();

    // in read&write mode, we fill cache before modify data. 
    // and at the very beginning, the cache block is empty.
    // but when eof, the cache block is empty too
    while (n > 0) {
        bool replace = readMode && mBlockLength;
        size_t m = MIN(n, replace ? 
                mBlock->ready() - mBlockOffset : 
                mBlock->capacity() - mBlockOffset);

        if (!m) {
            writeBlockBack();

            if (replace) readBlock();
            else {
                // reset cache manually
                mBlock->reset();
                mBlockOffset = 0;
            }
        } else {
            if (replace) {
                DEBUG("replace %zu bytes", m);
                mBlock->replace(mBlockOffset, 
                        s, m);
            } else {
                DEBUG("append %zu bytes", m);
                mBlock->write(s, m);
            }

            n -= m;
            s += m;
            mBlockOffset      += m;
            mBlockPopulated   = true;
        }
    }

    return (ssize_t)(data.ready() - n);
}

ssize_t Content::writeLine(const Buffer& line) {
    // TODO
    return 0;
}

int64_t Content::seek(int64_t offset) {
    DEBUG("real offset %" PRId64 ", cache offset %" PRId64 ", seek to %" PRId64, 
            mRangeOffset, 
            mBlockOffset, offset);

    if (mProto->flags() & Protocol::READ) {
        // read only mode or read & write mode.
        if (mBlock->ready() 
                && offset < mRangeOffset 
                && offset >= mRangeOffset - mBlock->ready()) {
            DEBUG("seek in cache.");
            mBlockOffset    = 
                mBlock->ready() - (mRangeOffset - offset);
            return offset;
        }
    } else {
        // write only mode
        if (mBlock->ready() 
                && offset >= mRangeOffset  
                && offset < mRangeOffset + mBlock->ready()) {
            DEBUG("write only mode: seek in cache.");
            mBlockOffset    = offset - mRangeOffset;
            return offset;
        }
    }

    flush();

    int64_t ret = mProto->seekBytes(
            mRangeStart + offset);

    if (ret < 0) {
        ERROR("seek return error %d", (int)ret);
        return ret;
    }

    mRangeOffset    = ret - mRangeStart;
    return mRangeOffset;
}

int64_t Content::size() const {
    return mRangeLength;
}

int64_t Content::tell() const {
    if (mBlock->ready() == 0) return mRangeOffset;

    if (mProto->flags() & Protocol::READ) {
        return mRangeOffset - (mBlock->ready() - mBlockOffset);
    } else {
        return mRangeOffset + mBlockOffset;
    }
}

int64_t Content::skip(int64_t bytes) {
    return seek(tell() + bytes);
}

void Content::setRange(int64_t offset, int64_t length) {
    //INFO("setRange %" PRId64 " %" PRId64, offset, length);

    mRangeStart     = offset;
    mRangeLength    = mProto->totalBytes() - mRangeStart;

    if (length > 0 && mRangeLength > length) 
        mRangeLength = length;

    // always reset real position
    seek(0);
}

void Content::reset() {
    setRange(0, mProto->totalBytes());
}

String Content::string() const {
    // TODO
    return String();
}

__END_NAMESPACE_ABE
