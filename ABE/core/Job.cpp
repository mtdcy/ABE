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


// File:    Job.cpp
// Author:  mtdcy.chen
// Changes:
//          1. 20160701     initial version
//

#define LOG_TAG   "Job"
//#define LOG_NDEBUG 0
#include "Log.h"
#include "Looper.h"

__BEGIN_NAMESPACE_ABE

Job::Job() : SharedObject(), mLooper(), mTicks(0) { }

Job::Job(const sp<Looper>& lp) : SharedObject(),
mLooper(lp), mTicks(0) { }

Job::Job(const sp<DispatchQueue>& disp) : SharedObject(),
mQueue(disp), mTicks(0) { }

Job::~Job() {
}

size_t Job::run(int64_t us) {
    if (!mLooper.isNIL())
        mLooper->post(this, us);
    else if (!mQueue.isNIL())
        mQueue->dispatch(this, us);
    else
        execution();
    return mTicks.load();
}

size_t Job::cancel() {
    if (!mLooper.isNIL())
        mLooper->remove(this);
    else if (!mQueue.isNIL())
        mQueue->remove(this);
    return mTicks.load();
}

void Job::execution() {
    onJob();
    mTicks++;
}

__END_NAMESPACE_ABE
