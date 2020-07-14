/******************************************************************************
 * Copyright (c) 2020, Chen Fang <mtdcy.chen@gmail.com>
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
#include "ABE/core/Log.h"

#include "core/Content.h"

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#ifndef O_LARGEFILE
#define O_LARGEFILE (0) // FIXME: find a better replacement
#endif 

#ifndef O_BINARY
#define O_BINARY    (0)
#endif

#ifndef HAVE_LSEEK64
#define lseek64 lseek
#endif

#define MIN(a, b) ((a) > (b)) ? (b) : (a)

__BEGIN_NAMESPACE_ABE

struct File : public Protocol {
    String          mUrl;
    eMode           mMode;  // read | write
    
    Int             mFd;
    Int64           mOffset;
    Int64           mLength;
    mutable Int64   mPosition;
    const UInt32    kBlockLength;
    mutable Char *  mBlock;
    
    File(const String& url, eMode mode) : Protocol(),
    mUrl(url), mMode(mode), mFd(-1), mOffset(0), mLength(0), mPosition(0),
    kBlockLength(4096), mBlock((Char *)malloc(kBlockLength))
    {
        CHECK_NULL(mBlock);
        if (url.startsWith("pipe://", True)) {
            Int64 offset, length;
            
            Int index0 = url.indexOf(7, "+");
            if (index0 < 7) return; // "+" not found.
            mFd     = url.substring(7, index0 - 7).toInt32();
            
            Int index1 = url.indexOf(index0 + 1, "+");
            mOffset = url.substring(index0 + 1, index1 - index0 - 1).toInt64();
            mLength = url.substring(index1 + 1).toInt64();
            
            INFO("fd = %d, offset = %lld, length = %lld", mFd, mOffset, mLength);
            
            if (mFd <= 0) {
                ERROR("invalid fd %d", mFd);
                return;
            }
            mFd = dup(mFd);
            
            mLength += mOffset;
            
            Int64 realLength = lseek64(mFd, 0, SEEK_END);
            if (realLength > 0 && mLength > realLength) {
                mLength = realLength;
            }
            
        } else {
            Int flags = O_LARGEFILE;
            if ((mode & Read) && (mode & Write)) {
                flags |= O_CREAT;
                flags |= O_RDWR;
                flags |= O_BINARY;
            } else if (mode & Read) {
                flags |= O_RDONLY;
                flags |= O_BINARY;
            } else if (mode & Write) {
                flags |= O_CREAT;
                flags |= O_WRONLY;
                flags |= O_BINARY;
                //flags |= O_TRUNC; // it's better to let client do this.
            }
            
            const Char *pathname = url.c_str();
            if (url.startsWith("file://", True))    pathname += 6;
            
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
    }
    
    ~File() {
        if (mFd >= 0) {
            ::close(mFd);
            mFd = -1;
        }
    }
    
    virtual Protocol::eMode mode() const {
        return mMode;
    }
    
    virtual UInt32 readBytes(sp<Buffer>& buffer) const {
        CHECK_GE(buffer->empty(), kBlockLength);
        
        Int64 i = 0;
        for (;;) {
            UInt32 bytes = MIN(kBlockLength, (mLength - mPosition));
            if (bytes > buffer->empty()) break;
            
            Int bytesRead = ::read(mFd, mBlock, bytes);
            if (bytesRead == 0) {
                INFO("End Of File");
                break;
            } else if (bytesRead < 0) {
                ERROR("read@%lld return error(%d|%s) %d/%d", mPosition,
                      errno, strerror(errno), bytesRead, bytes);
                break;
            }
            buffer->writeBytes(mBlock, (UInt32)bytesRead);
            i += (UInt32)bytesRead;
            mPosition += (Int64)bytesRead;
        }
        return (UInt32)i;
    }
    
    virtual UInt32 writeBytes(const sp<Buffer>& buffer) {
        
        Int bytesWritten = ::write(mFd, buffer->data(), buffer->size());
        
        if (bytesWritten < 0) {
            ERROR("write return error %d. errno = %d %s", bytesWritten,
                  errno, strerror(errno));
            return 0;
        }
        
        buffer->skipBytes(bytesWritten);
        mPosition += (Int64)bytesWritten;
        
        // we append more data.
        if (mPosition > mLength) {
            mLength += bytesWritten;
        }
        // else. we are writting in middle of file
        
        return (UInt32)bytesWritten;
    }
    
    virtual Int64 seekBytes(Int64 offset) const {
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
    
    virtual Int64 totalBytes() const {
        return mLength - mOffset;;
    }

    virtual UInt32 blockLength() const {
        return 4096;
    }
};

sp<Protocol> CreateFile(const String& url, File::eMode mode) {
    sp<File> file = new File(url, mode);
    if (file->mFd < 0) return Nil;
    return file;
}
__END_NAMESPACE_ABE
