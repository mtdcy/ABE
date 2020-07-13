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


#define LOG_TAG "ABE.c"
#include "ABE.h"

USING_NAMESPACE_ABE

BEGIN_DECLS

SharedObjectRef SharedObjectRetain(SharedObjectRef ref) {
    return static_cast<SharedObject *>(ref)->RetainObject();
}

void SharedObjectRelease(SharedObjectRef ref) {
    static_cast<SharedObject *>(ref)->ReleaseObject();
}

UInt32 SharedObjectGetRetainCount(const SharedObjectRef ref) {
    return static_cast<const SharedObject *>(ref)->GetRetainCount();
}

UInt32 SharedObjectGetID(const SharedObjectRef ref) {
    return static_cast<const SharedObject *>(ref)->GetObjectID();
}

AllocatorRef AllocatorGetDefault(void) {
    return (AllocatorRef)kAllocatorDefault->RetainObject();
}

AllocatorRef AllocatorGetDefaultAligned(UInt32 alignment) {
    return (AllocatorRef)GetAllocator(alignment)->RetainObject();
}

void * AllocatorAllocate(AllocatorRef ref, UInt32 n) {
    return static_cast<Allocator *>(ref)->allocate(n);
}

void * AllocatorReallocate(AllocatorRef ref, void * p, UInt32 n) {
    return static_cast<Allocator *>(ref)->reallocate(p, n);
}

void AllocatorDeallocate(AllocatorRef ref, void * p) {
    static_cast<Allocator *>(ref)->deallocate(p);
}

SharedBufferRef SharedBufferCreate(AllocatorRef allocator, UInt32 sz) {
    return SharedBuffer::Create(static_cast<Allocator*>(allocator), sz);
}

SharedBufferRef SharedBufferRetain(SharedBufferRef ref) {
    return static_cast<SharedBuffer *>(ref)->RetainBuffer();
}

UInt32 SharedBufferGetRetainCount(const SharedBufferRef ref) {
    return static_cast<const SharedBuffer *>(ref)->GetRetainCount();
}

void SharedBufferRelease(SharedBufferRef ref) {
    static_cast<SharedBuffer *>(ref)->ReleaseBuffer(False);
}

Char * SharedBufferGetDataPointer(SharedBufferRef ref) {
    return static_cast<SharedBuffer *>(ref)->data();
}

const Char * SharedBufferGetConstDataPointer(const SharedBufferRef ref) {
    return static_cast<const SharedBuffer *>(ref)->data();
}

UInt32 SharedBufferGetDataLength(const SharedBufferRef ref) {
    return static_cast<SharedBuffer *>(ref)->size();
}

SharedBufferRef SharedBufferEdit(SharedBufferRef ref) {
    return static_cast<SharedBuffer *>(ref)->edit();
}

SharedBufferRef SharedBufferEditWithSize(SharedBufferRef ref, UInt32 sz) {
    return static_cast<SharedBuffer *>(ref)->edit(sz);
}

UInt32 SharedBufferReleaseWithoutDelete(SharedBufferRef ref) {
    return static_cast<SharedBuffer *>(ref)->ReleaseBuffer(True);
}

void SharedBufferDelete(SharedBufferRef ref) {
    static_cast<SharedBuffer *>(ref)->DeleteBuffer();
}

BufferObjectRef BufferObjectCreate(UInt32 cap) {
    sp<ABuffer> buffer = new Buffer(cap);
    return buffer->RetainObject();
}

BufferObjectRef BufferObjectCreateWithUrl(const Char * url) {
    sp<ABuffer> buffer = Content::Create(String(url));
    return buffer->RetainObject();
}

Int64 BufferObjectGetCapacity(const BufferObjectRef ref) {
    return static_cast<const ABuffer *>(ref)->capacity();
}

Int64 BufferObjectGetDataLength(const BufferObjectRef ref) {
    return static_cast<const ABuffer *>(ref)->size();
}

Int64 BufferObjectGetEmptyLength(const BufferObjectRef ref) {
    return static_cast<const ABuffer *>(ref)->empty();
}

Int64 BufferObjectGetOffset(const BufferObjectRef ref) {
    return static_cast<const ABuffer *>(ref)->offset();
}

const Char * BufferObjectGetDataPointer(const BufferObjectRef ref) {
    return static_cast<const ABuffer *>(ref)->data();
}

UInt32 BufferObjectGetData(const BufferObjectRef ref, Char * data, UInt32 n) {
    return static_cast<const ABuffer *>(ref)->readBytes(data, n);
}

BufferObjectRef BufferObjectReadBytes(const BufferObjectRef ref, UInt32 n) {
    sp<ABuffer> buffer = static_cast<const ABuffer *>(ref)->readBytes(n);
    if (buffer.isNil()) return Nil;
    return buffer->RetainObject();
}

Int64 BufferObjectSkipBytes(const BufferObjectRef ref, Int64 offset) {
    return static_cast<const ABuffer *>(ref)->skipBytes(offset);
}

void BufferObjectResetBytes(const BufferObjectRef ref) {
    static_cast<const ABuffer *>(ref)->resetBytes();
}

BufferObjectRef BufferObjectCloneBytes(const BufferObjectRef ref) {
    return static_cast<const ABuffer *>(ref)->cloneBytes()->RetainObject();
}

UInt32 BufferObjectPutData(BufferObjectRef ref, const Char * data, UInt32 n) {
    return static_cast<ABuffer *>(ref)->writeBytes(data, n);
}

UInt32 BufferObjectWriteBytes(BufferObjectRef ref, BufferObjectRef data, UInt32 n) {
    return static_cast<ABuffer *>(ref)->writeBytes(static_cast<ABuffer*>(data), n);
}

void BufferObjectFlushBytes(BufferObjectRef ref) {
    static_cast<ABuffer *>(ref)->flushBytes();
}

void BufferObjectClearBytes(BufferObjectRef ref) {
    static_cast<ABuffer *>(ref)->clearBytes();
}

MessageObjectRef MessageObjectCreate() {
    sp<Message> message = new Message;
    return (MessageObjectRef)message->RetainObject();
}

MessageObjectRef MessageObjectCopy(const MessageObjectRef ref) {
    sp<Message> copy = static_cast<const Message *>(ref)->copy();
    return (MessageObjectRef)copy->RetainObject();
}

UInt32 MessageObjectGetCount(const MessageObjectRef ref) {
    return static_cast<const Message *>(ref)->size();
}

Bool MessageObjectContains(const MessageObjectRef ref, UInt32 name) {
    return static_cast<const Message *>(ref)->contains(name);
}

Bool MessageObjectRemove(MessageObjectRef ref, UInt32 name) {
    return static_cast<Message *>(ref)->remove(name);
}

void MessageObjectClear(MessageObjectRef ref) {
    static_cast<Message *>(ref)->clear();
}

#define MessageObjectPut(SUFFIX, DATA_TYPE)                                                     \
    void MessageObjectPut##SUFFIX(MessageObjectRef ref, UInt32 name, DATA_TYPE data) {    \
        static_cast<Message *>(ref)->set##SUFFIX(name, data);                                   \
    }

MessageObjectPut(Int32,     Int32);
MessageObjectPut(Int64,     Int64);
MessageObjectPut(Float,     Float32);
MessageObjectPut(Double,    Float64);
MessageObjectPut(Pointer,   void *);
MessageObjectPut(String,    const Char *);

void MessageObjectPutObject(MessageObjectRef ref, UInt32 name, SharedObjectRef obj) {
    sp<Message> message = static_cast<Message *>(ref);
    message->setObject(name, static_cast<SharedObject *>(obj));
}

#define MessageObjectGet(SUFFIX, DATA_TYPE)                                                             \
    DATA_TYPE MessageObjectGet##SUFFIX(const MessageObjectRef ref, UInt32 name, DATA_TYPE def) {  \
        return static_cast<const Message *>(ref)->find##SUFFIX(name, def);                              \
    }

MessageObjectGet(Int32,     Int32);
MessageObjectGet(Int64,     Int64);
MessageObjectGet(Float,     Float32);
MessageObjectGet(Double,    Float64);
MessageObjectGet(Pointer,   void *);
MessageObjectGet(String,    const Char *);

SharedObjectRef MessageObjectGetObject(const MessageObjectRef ref, UInt32 name, SharedObjectRef def) {
    return (SharedObjectRef)static_cast<const Message *>(ref)->findObject(name, static_cast<SharedObject *>(def));
}

//////////////////////////////////////////////////////////////////////////////////
struct UserJob : public Job {
    UserCallback    mCallback;
    void *          mUserContext;
    UserJob(UserCallback callback, void * user) : Job(),
    mCallback(callback), mUserContext(user) { }
    UserJob(const sp<Looper>& lp, UserCallback callback, void * user) :Job(lp),
    mCallback(callback), mUserContext(user) { }
    
    virtual void onJob() { mCallback(mUserContext); }
};

JobObjectRef  JobObjectCreate(UserCallback callback, void * user) {
    sp<Job> runnable = new UserJob(callback, user);
    return (Job *)runnable->RetainObject();
}

JobObjectRef  JobObjectCreateWithLooper(LooperObjectRef lp, UserCallback callback, void * user) {
    sp<Job> runnable = new UserJob(callback, user);
    return (Job *)runnable->RetainObject();
}

void JobObjectDispatch(JobObjectRef ref, UInt64 after) {
    static_cast<Job *>(ref)->dispatch(after);
}

Bool JobObjectSync(JobObjectRef ref, UInt64 deadline) {
    return static_cast<Job *>(ref)->sync(deadline);
}

void JobObjectCancel(JobObjectRef ref) {
    static_cast<Job *>(ref)->cancel();
}

LooperObjectRef LooperObjectMain() {
    return Looper::Main()->RetainObject();
}

LooperObjectRef LooperObjectCurrent() {
    return Looper::Current()->RetainObject();
}

LooperObjectRef LooperObjectCreate(const Char * name) {
    return (new Looper(name))->RetainObject();
}

void LooperObjectDispatch(LooperObjectRef ref, JobObjectRef job, UInt64 after) {
    static_cast<Looper *>(ref)->dispatch(static_cast<Job *>(job), after);
}

Bool LooperObjectSync(LooperObjectRef ref, JobObjectRef job, UInt64 deadline) {
    return static_cast<Looper *>(ref)->sync(static_cast<Job *>(job), deadline);
}

Bool LooperObjectRemove(LooperObjectRef ref, JobObjectRef job) {
    return static_cast<Looper *>(ref)->remove(static_cast<Job *>(job));
}

Bool LooperObjectFind(LooperObjectRef ref, JobObjectRef job) {
    return static_cast<Looper *>(ref)->exists(static_cast<Job *>(job));
}

void SharedLooperFlush(LooperObjectRef ref) {
    static_cast<Looper *>(ref)->flush();
}

DispatchQueueRef DispatchQueueCreate(LooperObjectRef ref) {
    return (new DispatchQueue(static_cast<Looper *>(ref)))->RetainObject();
}

void DispatchQueueDispatch(DispatchQueueRef ref, JobObjectRef job, UInt64 after) {
    static_cast<DispatchQueue *>(ref)->dispatch(static_cast<Job *>(job), after);
}

Bool DispatchQueueSync(DispatchQueueRef ref, JobObjectRef job, UInt64 deadline) {
    return static_cast<DispatchQueue *>(ref)->sync(static_cast<Job *>(job), deadline);
}

Bool DispatchQueueRemove(DispatchQueueRef ref, JobObjectRef job) {
    return static_cast<DispatchQueue *>(ref)->remove(static_cast<Job *>(job));
}

Bool DispatchQueueFind(DispatchQueueRef ref, JobObjectRef job) {
    return static_cast<DispatchQueue *>(ref)->exists(static_cast<Job *>(job));
}

void DispatchQueueFlush(DispatchQueueRef ref) {
    static_cast<DispatchQueue *>(ref)->flush();
}


END_DECLS
