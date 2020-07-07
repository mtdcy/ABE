/******************************************************************************
 * Copyright (c) 2018, Chen Fang <mtdcy.chen@gmail.com>
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

// File:    SharedBuffer.h
// Author:  mtdcy.chen
// Changes:
//          1. 20181112     initial version
//

#ifndef ABE_HEADERS_SHARED_BUFFER_H
#define ABE_HEADERS_SHARED_BUFFER_H
#include <ABE/core/Types.h>
#include <ABE/core/Allocator.h>

__BEGIN_NAMESPACE_ABE

/**
 * a cow buffer
 * @note DON'T use sp to hold SharedBuffer
 */
struct ABE_EXPORT SharedBuffer : protected SharedObject {
    protected:
        SharedBuffer();
        ~SharedBuffer() { }

    public:
        /**
         * create a cow buffer with given size and backend allocator
         */
        static SharedBuffer *       allocate(const sp<Allocator>&, UInt32);
        /**
         * manually release backend memory
         */
        void                        deallocate();

    public:
        /**
         * retain this cow buffer
         * @note retain is thread safe, can perform at any time
         */
        ABE_INLINE SharedBuffer *   RetainBuffer()              { return (SharedBuffer *)SharedObject::RetainObject(); }
    
        /**
         * release this cow buffer
         * @param keep  if keep == False & this is the last ref, backend memory will be released
         *              if keep == True & this is the last ref, manually release backend memory using deallocate
         *
         * @note release is thread safe, can perform at any time
         * @note if client have to do extra destruction work on this cow buffer, please always using keep = True
         */
        UInt32                      ReleaseBuffer(Bool keep = False);
    
        /**
         * get this cow buffer's ref count
         */
        ABE_INLINE UInt32           GetRetainCount() const      { return SharedObject::GetRetainCount();    }
    
        /**
         * is this cow buffer shared with others
         * @note if IsBufferNotShared() == True, it is safe to do anything later
         * @note if IsBufferShared() == True, you may have to test again later
         */
        ABE_INLINE Bool             IsBufferShared() const      { return SharedObject::IsObjectShared();    }
        ABE_INLINE Bool             IsBufferNotShared() const   { return SharedObject::IsObjectNotShared(); }

    public:
        ABE_INLINE Char *           data()                      { return mData;                             }
        ABE_INLINE const Char *     data() const                { return mData;                             }
        ABE_INLINE UInt32           size() const                { return mSize;                             }

    public:
        /**
         * perform edit before you write to this cow buffer
         * @note if this cow is not shared, it does NOTHING. otherwise it perform a cow action
         * @note edit with new size always perform a cow action.
         * @note edit with new size will assert on failure
         */
        SharedBuffer *              edit();
        SharedBuffer *              edit(UInt32);

    private:
        sp<Allocator>   mAllocator;
        Char *          mData;
        UInt32          mSize;
    
        DISALLOW_EVILS(SharedBuffer);
};

__END_NAMESPACE_ABE
#endif // ABE_HEADERS_SHARED_BUFFER_H
