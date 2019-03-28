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


// File:    File.h
// Author:  mtdcy.chen
// Changes: 
//          1. 20160701     initial version
//

#define LOG_TAG   ".MemoryProto"
#include <toolkit/Log.h>
#include <toolkit/Memory.h>

#include "MemoryProto.h"

#include <string.h> //strncmp
#include <stdlib.h>
#include <ctype.h> // isdigit

namespace mtdcy {

    MemoryProto::MemoryProto(const String& url, int mode)
        :
            Content::Protocol(),
            mMode(mode),
            mStatus(-1),
            mBuffer(NULL),
            mBufferLength(0),
            mBufferOffset(0)
    {
        if (url.startsWithIgnoreCase("buffer://")) {
            int64_t base = 0;
            int64_t length = 0;

            char *tmp = (char*)url.c_str() + 9;
            if (!strncasecmp(tmp, "0x", 2)) {
                base = strtoll(tmp + 2, &tmp, 16);
            } else {
                char *tmp2 = tmp;
                int hex = 0;
                while (*tmp2 != '+') {
                    if (!isdigit(*tmp2)) {
                        hex = 1;
                        break;
                    }
                    ++tmp2;
                }

                if (hex) 
                    base = strtoll(tmp, &tmp, 16);
                else
                    base = strtoll(tmp, &tmp, 10);
            }

            tmp += 1; // +
            length = strtoll(tmp, &tmp, 10);

            DEBUG("base %lld length %lld", base, length);

            mBuffer = (uint8_t*)base;
            mBufferLength = length;

            mStatus = 0;
        } else {
            ERROR("unknown url %s", url.c_str());
            mStatus = -1;
            return;
        }
    }

    MemoryProto::~MemoryProto() {
    }

#define MIN(a, b) (a > b ? b : a)

    ssize_t MemoryProto::readBytes(void *buffer, size_t bytes) {

        size_t copyBytes = MIN(bytes, mBufferLength - mBufferOffset);

        if (copyBytes > 0) {
            memcpy(buffer, mBuffer + mBufferOffset, copyBytes);

            mBufferOffset += copyBytes;
        }

        DEBUG("readBytes@%lld %d return %d", mBufferOffset, bytes, copyBytes);

        return (ssize_t)copyBytes;
    }

    ssize_t MemoryProto::writeBytes(const void *buffer, size_t bytes) {
        size_t writeBytes = MIN(bytes, mBufferLength - mBufferOffset);

        if (writeBytes > 0) {
            memcpy(mBuffer + mBufferOffset, buffer, writeBytes);

            mBufferOffset += writeBytes;
        }

        DEBUG("writeBytes %d return %d", bytes, writeBytes);

        return (ssize_t)writeBytes;
    }

    int64_t MemoryProto::totalBytes() const {
        DEBUG("totalBytes %lld", mBufferLength);
        return mBufferLength;
    }

    int64_t MemoryProto::seekBytes(int64_t offset) {

        mBufferOffset = offset;
        if (mBufferOffset < 0)
            mBufferOffset = 0;
        else if (mBufferOffset > mBufferLength)
            mBufferOffset = mBufferLength;

        DEBUG("seekBytes %lld result %lld", offset, mBufferOffset);

        return mBufferOffset;
    }
}
