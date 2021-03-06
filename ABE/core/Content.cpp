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

#include <string.h>  // memcpy

#define MIN(a, b)   ((a) > (b) ? (b) : (a))

__BEGIN_NAMESPACE_ABE

static size_t kMaxLineLength = 1024;

///////////////////////////////////////////////////////////////////////////
//! return false on error or eos
bool Content::readBlock() {
    writeBlockBack();

    // read cache first, and then read from cache.
    ssize_t ret = mProto->readBytes(mBlock->data(), mBlock->empty());
    if (ret == 0) {
        DEBUG("readBlock: eos or error ...");
        return false;
    }

    mBlock->step(ret);
    mBlockLength        = ret;
    mPosition           += ret;

    return true;
}

//! return true on success
bool Content::writeBlockBack() {
    if (mBlockPopulated && mBlock->ready() > 0) {
        CHECK_TRUE(mode() & Write);
        DEBUG("write back, real offset %" PRId64 ", cache offset %" PRId64, 
                mPosition, mBlockOffset);

        DEBUG("write back: %s", mBlock->string().c_str());

        int64_t offset = mPosition;
#if 0   // TODO
        // in read & write mode, which read cache first, 
        // and then modify cache, then write back
        // so, we have to seek back before write back
        const bool readMode = mode() & Read;
        if (readMode && mBlockLength) {
            offset  -= mBlockLength;
            mProto->seekBytes(offset);
        }
#endif
        
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
        mPosition    = offset + ret;
    }
    
    mBlock->reset();
    mBlockOffset        = 0;
    mBlockLength        = 0;
    mBlockPopulated     = false;

    return true;
}

///////////////////////////////////////////////////////////////////////////
// static
sp<Content::Protocol> CreateFile(const String& url, Content::eMode mode);

sp<Content> Content::Create(const String& url, eMode mode) {
    INFO("Open content %s", url.c_str());

    sp<Protocol> proto;
    if (url.startsWithIgnoreCase("file://") || 
            url.startsWithIgnoreCase("android://") ||
            url.startsWithIgnoreCase("pipe://")) {
        proto = CreateFile(url, mode);
    } else {
        // normal file path
        proto = CreateFile(url, mode);
    }

    if (proto.isNIL()) {
        ERROR("unsupported url %s", url.c_str());
        return NULL;
    }

    return Content::Create(proto);
}

sp<Content> Content::Create(const sp<Protocol>& proto) {
    return new Content(proto, proto->blockLength());
}

// read mode: read - read - read
// write mode: write - write - write
// read & write mode: read - write - write - ... - writeBack
Content::Content(const sp<Protocol>& proto, size_t blockLength) :
    mProto(proto),
    mPosition(0),
    mBlock(new Buffer(blockLength)), mBlockOffset(0), 
    mBlockLength(0), mBlockPopulated(false)
{
}

Content::~Content() {
    writeBlockBack();
}

sp<Buffer> Content::read(size_t size) {
    CHECK_TRUE(mode() & Read);

    sp<Buffer> data = new Buffer(size);

    //bool eof = false;
    size_t n = size;

    while (n > 0) {
        size_t remains = mBlock->ready() - mBlockOffset;
        char *s = mBlock->data() + mBlockOffset;
        if (remains > 0) {
            size_t m = MIN(remains, n);
            //DEBUG("read %d bytes from cache.", m);

            data->write(s, m);

            mBlockOffset  += m;
            n -= m;
        } else {
            // TODO: read directly from protocol 
            if (readBlock() == false) {
                //eof = true;
                break;
            }
        } 
    }

    n               = size - n;

    if (!n) return NULL;

    //DEBUG("%s", PRINTABLE(data->string()));

    return data;
}

size_t Content::write(const char *data, size_t size) {
    CHECK_TRUE(mode() & Write);
    
    // TODO: write in Read&Write mode
    CHECK_FALSE(mode() & Read);
    //const bool readMode = mode() & Read;

    size_t n = size;
    while (n) {
        size_t m = MIN(n, mBlock->empty() - mBlockOffset);

        if (!m) {
            writeBlockBack();
        } else {
            mBlock->write(data, m);

            n       -= m;
            data    += m;
            mBlockPopulated = true;
        }
    }

    return size - n;
}

int64_t Content::seek(int64_t offset) {
    DEBUG("real offset %" PRId64 ", cache offset %" PRId64 ", seek to %" PRId64, 
            mPosition,
            mBlockOffset, offset);

    if (mode() & Read) {
        // read only mode or read & write mode.
        if (mBlock->ready() 
                && offset < mPosition
                && offset >= mPosition - (int64_t)mBlock->ready()) {
            DEBUG("seek in cache.");
            mBlockOffset    = 
                mBlock->ready() - (mPosition - offset);
            return offset;
        }
    } else {
        // write only mode
        if (mBlock->ready() 
                && offset >= mPosition
                && offset < mPosition + (int64_t)mBlock->ready()) {
            DEBUG("write only mode: seek in cache.");
            mBlockOffset    = offset - mPosition;
            return offset;
        }
    }

    writeBlockBack();

    mPosition = mProto->seekBytes(offset);
    return mPosition;
}

int64_t Content::length() const {
    return mProto->totalBytes();
}

int64_t Content::tell() const {
    if (mBlock->ready() == 0) return mPosition;

    if (mode() & Read) {
        return mPosition - (mBlock->ready() - mBlockOffset);
    } else {
        return mPosition + mBlockOffset;
    }
}

int64_t Content::skip(int64_t pos) {
    return seek(tell() + pos);
}

__END_NAMESPACE_ABE

