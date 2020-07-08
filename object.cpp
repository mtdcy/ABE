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


// File:    sb.cpp
// Author:  mtdcy.chen
// Changes:
//          1. 20200708     initial version

#define LOG_TAG "sb.test"
#include <ABE/ABE.h>

USING_NAMESPACE_ABE

struct SimpleSharedObject : public SharedObject {
    const String    mName;
    void *          mData;
    Atomic<UInt32>  mState;
    SimpleSharedObject(const String& name) : SharedObject(FOURCC('sobj')),
    mName(name), mData(Nil), mState(0) {
        INFO("+SimpleSharedObject");
        CHECK_EQ(GetRetainCount(), 0);
    }
    
    virtual void onFirstRetain() {
        INFO("+onFirstRetain");
        CHECK_EQ(++mState, 1);
        mData = malloc(1);
        CHECK_EQ(GetRetainCount(), 1);
    }
    
    virtual void onLastRetain() {
        INFO("-onLastRetain");
        CHECK_EQ(++mState, 2);
        free(mData);
        mData = Nil;
        CHECK_EQ(GetRetainCount(), 0);
    }
    
    ~SimpleSharedObject() {
        INFO("-SimpleSharedObject");
        UInt32 state = ++mState;
        // case 3/4: state == 1
        CHECK_TRUE(state == 3 || state == 1);
        CHECK_EQ(mData, Nil);
        CHECK_EQ(GetRetainCount(), 0);
    }
};

struct MyJob : public Job {
    virtual void onJob() {
        
    }
};

int main(int argc, char ** argv) {
    {
        MemoryAnalyzer MA;
        INFO("case 1");
        // case 1: only strong refs
        sp<SharedObject> sp0 = new SimpleSharedObject("case 1");
        sp<SharedObject> sp1 = sp0;
    }
    
    {
        MemoryAnalyzer MA;
        INFO("case 2 sub 1");
        // case 2: both strong & weak refs;
        sp<SharedObject> sp0 = new SimpleSharedObject("case 2 sub 1");
        sp<SharedObject> sp1 = sp0;
        wp<SharedObject> wp0 = sp0;
        wp<SharedObject> wp1 = wp0;
    }
    
    {
        MemoryAnalyzer MA;
        INFO("case 2 sub 2");
        // case 2: both strong & weak refs;
        wp<SharedObject> wp0 = new SimpleSharedObject("case 2 sub 2");
        wp<SharedObject> wp1 = wp0;
        sp<SharedObject> sp0 = wp0.retain();
        sp<SharedObject> sp1 = sp0;
    }
    
    {
        MemoryAnalyzer MA;
        INFO("case 3");
        // case 3: only weak refs
        wp<SharedObject> wp0 = new SimpleSharedObject("case 3");
        wp<SharedObject> wp1 = wp0;
    }
    
    {
        MemoryAnalyzer MA;
        INFO("case 4");
        // case 4: no strong or weak refs
        SharedObject * object = new SimpleSharedObject("case 4");
        delete object;
    }
    
    {
        MemoryAnalyzer MA;
        INFO("shared buffer");
        SharedBuffer * sb0 = SharedBuffer::Create(kAllocatorDefault, 1);
        SharedBuffer * sb1 = sb0->RetainBuffer();   // retain
        sb0 = sb0->edit();  // cow
        sb0->ReleaseBuffer();
        sb1->ReleaseBuffer();
    }
    
    return 0;
}
