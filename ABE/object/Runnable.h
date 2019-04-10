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


// File:    Runnable.h
// Author:  mtdcy.chen
// Changes:
//          1. 20160701     initial version
//

#ifndef _TOOLKIT_HEADERS_RUNNABLE_H
#define _TOOLKIT_HEADERS_RUNNABLE_H

#ifdef __cplusplus
#include <ABE/basic/SharedObject.h>
#include <ABE/basic/Mutex.h>
__BEGIN_NAMESPACE_ABE

struct __ABE_EXPORT Runnable : public SharedObject {
    virtual void run() = 0;

    __OBJECT_DECLS(Runnable);
    DISALLOW_EVILS(Runnable);
};

class __ABE_EXPORT SyncRunnable : public Runnable {
    private:
        mutable Mutex   mLock;
        Condition       mWait;
        volatile int    mSync;
    
    protected:
        __ABE_INLINE SyncRunnable() : Runnable(), mSync(0) { }
    
    public:
        virtual void sync() = 0;
    
        /**
         * wait until finished
         * @param ns    timeout, always > 0
         * @return return true on success, else return false on timeout
         */
        __ABE_INLINE bool wait(int64_t ns = 0) {
            AutoLock _l(mLock);
            bool success = true;
            if (ns < 0) ns = 0;
            if (mSync == 0) {
                if (ns) success = !mWait.waitRelative(mLock, ns);
                else    mWait.wait(mLock);
            }
            if (success) mSync -= 1;
            return success;
        }
    
    protected:
        virtual void run() {
            sync();
            AutoLock _l(mLock);
            mSync += 1;
            mWait.broadcast();
        }
    
    
};

__END_NAMESPACE_ABE
#endif // __cplusplus

#endif // _TOOLKIT_HEADERS_RUNNABLE_H
