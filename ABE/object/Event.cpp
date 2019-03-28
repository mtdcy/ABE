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


// File:    Event.cpp
// Author:  mtdcy.chen
// Changes: 
//          1. 20160701     initial version
//

#define LOG_TAG     "Event"
//#define LOG_NDEBUG 0
#include "ABE/basic/Log.h"

#include "Event.h"

__BEGIN_NAMESPACE_ABE

struct SharedEvent : public SharedObject {
    String      mName;
    Event *     mEvent;
    sp<Looper>  mLooper;

    SharedEvent(Event * event, const sp<Looper>& looper) :
        SharedObject(),
        mEvent(event), mLooper(looper) { }
    SharedEvent(const String& name, Event * event, const sp<Looper>& looper) :
        SharedObject(), mName(name),
        mEvent(event), mLooper(looper) { }
};

struct Event::EventRunnable : public Runnable {
    SharedEvent *  mShared;
    EventRunnable(SharedEvent *shared) : Runnable(), mShared(shared) {
        mShared->RetainObject();
    }
    virtual ~EventRunnable() { mShared->ReleaseObject(); }

    virtual void run() {
        // this only fix the 'Pure virtual function called!' problem
        // it is still NOT thread safe.
        if (!mShared->IsObjectShared()) {
            ERROR("context[%s] went away before execution", mShared->mName.c_str());
        } else {
            // FIXME: NOT thread safe
            // mEvent may be released by others at this moment
            mShared->mEvent->onEvent();
        }
    }
};

Event::Event() : mShared(NULL) {
}

Event::Event(const sp<Looper>& looper) :
    mShared(new SharedEvent(this, looper))  {
        mShared->RetainObject();
    }

Event::Event(const String& name, const sp<Looper>& looper) :
    mShared(new SharedEvent(name, this, looper)) {
        mShared->RetainObject();
    }

Event::~Event() {
    if (mShared) {
        mShared->ReleaseObject();
        mShared = NULL;
    }
}

void Event::fire(int64_t delay) {
    SharedEvent *shared = static_cast<SharedEvent*>(mShared);
    if (shared != NULL && shared->mLooper != NULL) {
        shared->mLooper->post(new EventRunnable((SharedEvent *)mShared), delay);
    } else {
        onEvent();
    }
}

__END_NAMESPACE_ABE
