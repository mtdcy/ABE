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


#ifndef _TOOLKIT_HEADERS_CONTENT_H
#define _TOOLKIT_HEADERS_CONTENT_H

#include <ABE/basic/Types.h>
#include <ABE/object/Buffer.h>
#include <ABE/basic/String.h>
#include <ABE/stl/List.h>

__BEGIN_NAMESPACE_ABE

enum eContentFlagType {
    kContentFlagRead    = 0x1,
    kContentFlagWrite   = 0x2,
    kContentFlagStream  = 0x4,
};

// NOTE: 
// 1. the Content object is not thread safe
class __ABE_EXPORT Content : public SharedObject {
    public:
        // it is ok to be lock free
        struct __ABE_EXPORT Protocol : public SharedObject {
            Protocol() { }
            virtual ~Protocol() { }
            virtual status_t    status() const = 0;
            virtual uint32_t    flags() const = 0;
            virtual ssize_t     readBytes(void *buffer, size_t bytes) = 0;
            virtual ssize_t     writeBytes(const void *buffer, size_t bytes) = 0;
            virtual int64_t     totalBytes() const = 0;
            virtual int64_t     seekBytes(int64_t offset) = 0;

            enum { READ = 0x1, WRITE = 0x2, STREAM = 0x4 };
        };

    public: 
        static Object<Content> Create(const String& url, uint32_t mode = Protocol::READ);

        Content(const Object<Protocol>& proto, size_t blockLength = 4096);

        ~Content();

        status_t        status() const;

        uint32_t        flags() const;  // @Protocol::flags

        String          string() const;

    public:
        //! read bytes from content 
        Object<Buffer>  read(size_t size);

        //! write bytes to content
        ssize_t         write(const Buffer& buffer);

        //! read line from content including the terminating null byte 
        //! excluding the return byte 
        String          readLine();

        //! write line to content
        ssize_t         writeLine(const Buffer& line);

        int64_t         seek(int64_t pos);

        int64_t         size() const;

        int64_t         tell() const;

        int64_t         skip(int64_t bytes);

        void            flush(); // write cache back

        bool            isStream() const { return flags() & Protocol::STREAM; }

    public:
        // TODO: find a better way to do this.
        void            setRange(int64_t offset, int64_t length = -1);
        void            reset();

    private:
        Object<Protocol>        mProto;

        int64_t                 mRangeStart;
        int64_t                 mRangeLength;
        int64_t                 mRangeOffset;

        Object<Buffer>          mBlock;         // cache block
        size_t                  mBlockOffset;   // offset shared by read and write
        size_t                  mBlockLength;   // how may bytes of cache in mBlock
        // which need to write back
        bool                    mBlockPopulated;    // need write back

        int64_t                 mRealOffset;

        // statistic 
        int                     mReadCnt;
        int                     mRealReadCnt;
        int64_t                 mReadBytes;
        int64_t                 mRealReadBytes;

        int                     mWriteCnt;
        int                     mRealWriteCnt;
        int64_t                 mWriteBytes;
        int64_t                 mRealWriteBytes;

        int                     mSeekCnt;
        int                     mRealSeekCnt;

        int                     mSkipCnt;
        int                     mSkipSeekCnt;

    private:
        bool                    readBlock();
        bool                    writeBlockBack();

    private:
        DISALLOW_EVILS(Content);
};

__END_NAMESPACE_ABE

#endif // _TOOLKIT_HEADERS_CONTENT_H

