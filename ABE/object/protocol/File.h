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

#ifndef _TOOLKIT_HEADERS_PIPES_FILE_H
#define _TOOLKIT_HEADERS_PIPES_FILE_H

#include "ABE/object/Content.h"

__BEGIN_NAMESPACE_ABE

namespace content_protocol __ABE_HIDDEN {
    class __ABE_HIDDEN File : public Content::Protocol {
        public:
            File(const String& url, int mode);

            virtual ~File();

            virtual status_t status() const { return mStatus; }

            virtual uint32_t flags() const { return mMode; }

            virtual ssize_t readBytes(void *buffer, size_t bytes);

            virtual ssize_t writeBytes(const void *buffer, size_t bytes);

            virtual int64_t totalBytes() const;

            virtual int64_t seekBytes(int64_t offset);

            virtual String string() const {
                return String::format("File %d[%#x]@%s", mFd, mMode, mUrl.c_str());
            }

        private:
            String          mUrl;
            int             mMode;  // read | write
            status_t        mStatus;

            int             mFd;
            int64_t         mOffset;
            int64_t         mLength;
            int64_t         mPosition;

        private:
            DISALLOW_EVILS(File);
    };
};

__END_NAMESPACE_ABE

#endif // _TOOLKIT_HEADERS_PIPES_FILE_H
