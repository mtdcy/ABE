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


#ifndef ABE_HEADERS_CONTENT_H
#define ABE_HEADERS_CONTENT_H

#include <ABE/core/Types.h>
#include <ABE/core/Buffer.h>

__BEGIN_NAMESPACE_ABE


/**
 * a content manager
 * @note not thread safe
 */
class ABE_EXPORT Content : public ABuffer {
    public:
        enum eMode {
            Read        = 0x1,              ///< read only
            Write       = 0x2,              ///< write only
            ReadWrite   = Read | Write,     ///< read & write
            Default     = Read              ///< default mode
        };
    
    public:
        /**
         * protocol interface
         */
        struct ABE_EXPORT Protocol : public SharedObject {
            /**
             * return mode of the protocol
             * @return see @Content::eMode
             */
            virtual eMode   mode() const = 0;
            
            /**
             * read bytes from protocol
             * @param buffer pointer to memory
             * @param length number bytes to read
             * @return return bytes read, otherwise return 0 on eos or error
             */
            virtual size_t  readBytes(sp<Buffer>& buffer) const = 0;

            /**
             * write bytes to protocol
             * @param buffer pointer to memory
             * @param length number bytes to write
             * @return return bytes written, otherwise return 0
             */
            virtual size_t  writeBytes(const sp<Buffer>& buffer) = 0;
            
            /**
             * get total bytes of the protocol
             * @return return number bytes of the protocol, otherwise return -1 if unknown
             */
            virtual int64_t totalBytes() const = 0;
            
            /**
             * seek to absolute position of the protocol
             * @param pos   absolute position to seek to
             * @return return the position after seek, otherwise return -1 on error
             */
            virtual int64_t seekBytes(int64_t pos) const = 0;

            /**
             * get block length of this protocol
             */
            virtual size_t blockLength() const = 0;
        };

    public:
        /**
         * create a content object
         * @param url url to content object
         * @param mode mode of the content object
         * @return return a new content object
         */
        static sp<Content> Create(const String& url, eMode mode = Default);

        /**
         * create a content object with custom protocol
         * @param proto the custom protocol
         * @return return a new content object 
         */
        static sp<Content> Create(const sp<Protocol>& proto);

    protected:
        Content(const sp<Protocol>& proto, size_t blockLength);

        ~Content();
    
    public:
        /**
         * return mode of the content
         * @return see @eMode
         */
        ABE_INLINE eMode    mode() const { return mProto->mode(); }
    
    public:
        virtual int64_t     capacity() const;
        virtual int64_t     size() const;
        virtual int64_t     empty() const;
        virtual int64_t     offset() const;
        virtual const char* data() const;
    
    public:
        virtual sp<ABuffer> readBytes(size_t) const;
        virtual size_t      readBytes(char *, size_t) const;
        virtual int64_t     skipBytes(int64_t) const;
        virtual void        resetBytes() const;
        virtual sp<ABuffer> cloneBytes() const;
    
        virtual size_t      writeBytes(const char *, size_t n = 0);
        virtual size_t      writeBytes(const sp<ABuffer>& b, size_t n = 0);
        virtual size_t      writeBytes(int c, size_t n);
        virtual void        flushBytes();
        virtual void        clearBytes();

    protected:
        virtual uint8_t     readByte() const;
        virtual void        writeByte(uint8_t);

    private:
        void                prepareBlock(size_t n) const;
        void                writeBlockBack(bool force = false);

    private:
        sp<Protocol>        mProto;
        mutable int64_t     mReadPosition;
        mutable sp<Buffer>  mReadBlock;
        int64_t             mWritePosition;
        sp<Buffer>          mWriteBlock;
    
        DISALLOW_EVILS(Content);
};

__END_NAMESPACE_ABE

#endif // ABE_HEADERS_CONTENT_H

