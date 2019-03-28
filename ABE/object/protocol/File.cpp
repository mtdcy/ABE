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

#define LOG_TAG "File"
#include "ABE/basic/Log.h"
#include "ABE/object/Content.h"

#include "Config.h" 
#include "File.h" 

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h> 

#ifndef O_LARGEFILE
#define O_LARGEFILE (0) // FIXME: find a better replacement
#endif 

#ifndef O_BINARY
#define O_BINARY    (0)
#endif

#ifndef HAVE_LSEEK64
#define lseek64 lseek
#endif

__BEGIN_NAMESPACE_ABE

namespace content_protocol {
    File::File(const String& url, int mode) 
        :
            Content::Protocol(),
            mUrl(url),
            mMode(mode),
            mStatus(NO_INIT),
            mFd(-1), 
            mOffset(0), 
            mLength(0),
            mPosition(0)
    {
        if (url.startsWithIgnoreCase("pipe://")) {
            int64_t offset, length;

            int index0 = url.indexOf(7, "+");
            if (index0 < 7) return; // "+" not found.
            mFd     = url.substring(7, index0 - 7).toInt32();

            int index1 = url.indexOf(index0 + 1, "+");
            mOffset = url.substring(index0 + 1, index1 - index0 - 1).toInt64();
            mLength = url.substring(index1 + 1).toInt64();

            INFO("fd = %d, offset = %lld, length = %lld", mFd, mOffset, mLength);

            if (mFd <= 0) {
                ERROR("invalid fd %d", mFd);
                return;
            }
            mFd = dup(mFd);

            mLength += mOffset;

            int64_t realLength = lseek64(mFd, 0, SEEK_END);
            if (realLength > 0 && mLength > realLength) {
                mLength = realLength;
            }

        } else {
            int flags = O_LARGEFILE;
            if ((mode & READ) && (mode & WRITE)) {
                flags |= O_CREAT;
                flags |= O_RDWR;
                flags |= O_BINARY;
            } else if (mode & READ) {
                flags |= O_RDONLY;
                flags |= O_BINARY;
            } else if (mode & WRITE) {
                flags |= O_CREAT;
                flags |= O_WRONLY;
                flags |= O_BINARY;
                //flags |= O_TRUNC; // it's better to let client do this.
            }

            const char *pathname = url.c_str();
            if (url.startsWithIgnoreCase("file://"))    pathname += 6;

            if (flags & O_CREAT) 
                mFd = ::open(pathname, flags, S_IRUSR | S_IWUSR);
            else
                mFd = ::open(pathname, flags);

            if (mFd < 0) {
                ERROR("open %s failed. errno = %d(%s)", url.c_str(), errno, strerror(errno));
                return;
            }

            mOffset = 0;
            mLength = ::lseek64(mFd, 0, SEEK_END);
        } 

        mPosition = lseek64(mFd, mOffset, SEEK_SET);
        DEBUG("mOffset = %" PRId64 ", mPosition = %" PRId64 " mLength = %" PRId64, mOffset, mPosition, mLength);

        mStatus     = OK;
    }

    File::~File() {
        if (mFd >= 0) {
            ::close(mFd);
            mFd = -1;
        }
    }

    ssize_t File::readBytes(void *buffer, size_t bytes) {
        int64_t remains = mLength - mPosition;

        if (remains == 0) {
            return 0;
        }

        if (bytes > remains) {
            DEBUG("read reamins %d bytes", remains);
            bytes = remains;
        }

        ssize_t bytesRead = ::read(mFd, buffer, bytes);

        if (bytesRead >= 0)
            mPosition += bytesRead;
        else {
            ERROR("read@%lld return error(%d|%s) %d/%d", mPosition, 
                    errno, strerror(errno),
                    bytesRead, bytes);
        }

        return bytesRead;
    }

    ssize_t File::writeBytes(const void *buffer, size_t bytes) {

        ssize_t bytesWritten = ::write(mFd, buffer, bytes);

        if (bytesWritten < 0) {
            ERROR("write return error %d. errno = %d %s", bytesWritten, 
                    errno, strerror(errno));
            return bytesWritten;
        }

        mPosition += (int64_t)bytesWritten;

        // we append more data.
        if (mPosition > mLength) {
            mLength   += bytesWritten;
        }
        // else. we are writting in middle of file

        return bytesWritten;
    }

    int64_t File::seekBytes(int64_t offset) {
        offset += mOffset;

        if (offset > mLength) {
            DEBUG("seek heet boundary %lld", mOffset);
            offset = mLength - mOffset;
        } else if (offset < 0) {
            DEBUG("seek heet boundary 0");
            offset = 0;
        }

        mPosition = ::lseek64(mFd, offset, SEEK_SET);

        return mPosition - mOffset;
    }

    int64_t File::totalBytes() const {
        return mLength - mOffset;;
    }

};

__END_NAMESPACE_ABE
