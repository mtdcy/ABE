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
#include <ABE/basic/SharedObject.h>
#include <ABE/object/Buffer.h>
#include <ABE/tools/Bits.h>

__BEGIN_NAMESPACE_ABE


/**
 * a content manager
 * @note not thread safe
 */
class __ABE_EXPORT Content : public SharedObject {
    public:
        /**
         * protocol interface
         */
        struct __ABE_EXPORT Protocol : public SharedObject {
            enum { Read = 0x1, Write = 0x2, Default = Read };
            ///< get protocol mode
            virtual uint32_t    mode() const = 0;
            ///< read bytes from protocol, return 0 on eos or error
            virtual size_t      readBytes(void * buffer, size_t bytes) = 0;
            ///< write bytes to protocol, return 0 on error
            virtual size_t      writeBytes(const void * buffer, size_t bytes) = 0;
            ///< get protocol total bytes
            virtual int64_t     totalBytes() const = 0;
            ///< seek protocol to abs position
            virtual int64_t     seekBytes(int64_t pos) = 0;
        };

    public: 
        static Object<Content> Create(const String& url, uint32_t mode = Protocol::Default);

        Content(const Object<Protocol>& proto, size_t blockLength = 4096);

        ~Content();
    
    public:
        /**
         * get content mode
         * @see Protocol
         */
        uint32_t        mode() const { return mProto->mode(); }
    
        /**
         * get content length in bytes
         */
        int64_t         length() const;
    
        /**
         * get content current position in bytes
         * @return
         */
        int64_t         tell() const;
    
    public:
        /**
         * seek to a absolute position in bytes
         * @note pos always be positive
         */
        int64_t         seek(int64_t pos);
    
        /**
         * seek to a relative position in bytes
         * @note pos can be positive and negtive
         */
        int64_t         skip(int64_t pos);

    public:
        /**
         * read bytes from content
         */
        Object<Buffer>  read(size_t size);

#if 0   // TODO
        /**
         * write bytes to content
         * @return
         */
        size_t          write(const Buffer& buffer);
#endif
    private:
        bool            readBlock();
        bool            writeBlockBack();

    private:
        Object<Protocol>    mProto;
        int64_t             mPosition;
        Object<Buffer>      mBlock;         // cache block
        size_t              mBlockOffset;   // offset shared by read and write
        size_t              mBlockLength;   // how may bytes of cache in mBlock
        bool                mBlockPopulated;    // need write back ?
    
    private:
        DISALLOW_EVILS(Content);
};

__END_NAMESPACE_ABE

#endif // _TOOLKIT_HEADERS_CONTENT_H

