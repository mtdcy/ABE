/******************************************************************************
 * Copyright (c) 2020, Chen Fang <mtdcy.chen@gmail.com>
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

__BEGIN_NAMESPACE_ABE

#if 0
// function static variable initialize guard.
ABE_EXPORT_C Int __cxa_guard_acquire(void *) {
    // TODO
}

ABE_EXPORT_C void __cxa_guard_release(void *) {
    // TODO
}
#endif

//** c bindings **//
ABE_EXPORT_C SharedObjectRef SharedObjectRetain(SharedObjectRef ref) {
    return static_cast<SharedObject *>(ref)->RetainObject();
}

ABE_EXPORT_C void SharedObjectRelease(SharedObjectRef ref) {
    static_cast<SharedObject *>(ref)->ReleaseObject();
}

ABE_EXPORT_C UInt32 SharedObjectGetRetainCount(const SharedObjectRef ref) {
    return static_cast<const SharedObject *>(ref)->GetRetainCount();
}

ABE_EXPORT_C UInt32 SharedObjectGetID(const SharedObjectRef ref) {
    return static_cast<const SharedObject *>(ref)->GetObjectID();
}

ABE_EXPORT_C AllocatorRef AllocatorGetDefault(void) {
    return (AllocatorRef)kAllocatorDefault->RetainObject();
}

ABE_EXPORT_C AllocatorRef AllocatorGetDefaultAligned(UInt32 alignment) {
    return (AllocatorRef)GetAllocator(alignment)->RetainObject();
}

ABE_EXPORT_C void * AllocatorAllocate(AllocatorRef ref, UInt32 n) {
    return static_cast<Allocator *>(ref)->allocate(n);
}

ABE_EXPORT_C void * AllocatorReallocate(AllocatorRef ref, void * p, UInt32 n) {
    return static_cast<Allocator *>(ref)->reallocate(p, n);
}

ABE_EXPORT_C void AllocatorDeallocate(AllocatorRef ref, void * p) {
    static_cast<Allocator *>(ref)->deallocate(p);
}

ABE_EXPORT_C SharedBufferRef SharedBufferCreate(AllocatorRef allocator, UInt32 sz) {
    return SharedBuffer::Create(static_cast<Allocator*>(allocator), sz);
}

ABE_EXPORT_C SharedBufferRef SharedBufferRetain(SharedBufferRef ref) {
    return static_cast<SharedBuffer *>(ref)->RetainBuffer();
}

ABE_EXPORT_C UInt32 SharedBufferGetRetainCount(const SharedBufferRef ref) {
    return static_cast<const SharedBuffer *>(ref)->GetRetainCount();
}

ABE_EXPORT_C void SharedBufferRelease(SharedBufferRef ref) {
    static_cast<SharedBuffer *>(ref)->ReleaseBuffer(False);
}

ABE_EXPORT_C Char * SharedBufferGetDataPointer(SharedBufferRef ref) {
    return static_cast<SharedBuffer *>(ref)->data();
}

ABE_EXPORT_C const Char * SharedBufferGetConstDataPointer(const SharedBufferRef ref) {
    return static_cast<const SharedBuffer *>(ref)->data();
}

ABE_EXPORT_C UInt32 SharedBufferGetDataLength(const SharedBufferRef ref) {
    return static_cast<SharedBuffer *>(ref)->size();
}

ABE_EXPORT_C SharedBufferRef SharedBufferEdit(SharedBufferRef ref) {
    return static_cast<SharedBuffer *>(ref)->edit();
}

ABE_EXPORT_C SharedBufferRef SharedBufferEditWithSize(SharedBufferRef ref, UInt32 sz) {
    return static_cast<SharedBuffer *>(ref)->edit(sz);
}

ABE_EXPORT_C UInt32 SharedBufferReleaseWithoutDelete(SharedBufferRef ref) {
    return static_cast<SharedBuffer *>(ref)->ReleaseBuffer(True);
}

ABE_EXPORT_C void SharedBufferDelete(SharedBufferRef ref) {
    static_cast<SharedBuffer *>(ref)->DeleteBuffer();
}

ABE_EXPORT_C BufferObjectRef BufferObjectCreate(UInt32 cap) {
    sp<ABuffer> buffer = new Buffer(cap);
    return buffer->RetainObject();
}

ABE_EXPORT_C BufferObjectRef BufferObjectCreateWithUrl(const Char * url) {
    sp<ABuffer> buffer = Content::Create(String(url));
    return buffer->RetainObject();
}

ABE_EXPORT_C Int64 BufferObjectGetCapacity(const BufferObjectRef ref) {
    return static_cast<const ABuffer *>(ref)->capacity();
}

ABE_EXPORT_C Int64 BufferObjectGetDataLength(const BufferObjectRef ref) {
    return static_cast<const ABuffer *>(ref)->size();
}

ABE_EXPORT_C Int64 BufferObjectGetEmptyLength(const BufferObjectRef ref) {
    return static_cast<const ABuffer *>(ref)->empty();
}

ABE_EXPORT_C Int64 BufferObjectGetOffset(const BufferObjectRef ref) {
    return static_cast<const ABuffer *>(ref)->offset();
}

ABE_EXPORT_C const Char * BufferObjectGetDataPointer(const BufferObjectRef ref) {
    return static_cast<const ABuffer *>(ref)->data();
}

ABE_EXPORT_C UInt32 BufferObjectGetData(const BufferObjectRef ref, Char * data, UInt32 n) {
    return static_cast<const ABuffer *>(ref)->readBytes(data, n);
}

ABE_EXPORT_C BufferObjectRef BufferObjectReadBytes(const BufferObjectRef ref, UInt32 n) {
    sp<ABuffer> buffer = static_cast<const ABuffer *>(ref)->readBytes(n);
    if (buffer.isNil()) return Nil;
    return buffer->RetainObject();
}

ABE_EXPORT_C Int64 BufferObjectSkipBytes(const BufferObjectRef ref, Int64 offset) {
    return static_cast<const ABuffer *>(ref)->skipBytes(offset);
}

ABE_EXPORT_C void BufferObjectResetBytes(const BufferObjectRef ref) {
    static_cast<const ABuffer *>(ref)->resetBytes();
}

ABE_EXPORT_C BufferObjectRef BufferObjectCloneBytes(const BufferObjectRef ref) {
    return static_cast<const ABuffer *>(ref)->cloneBytes()->RetainObject();
}

ABE_EXPORT_C UInt32 BufferObjectPutData(BufferObjectRef ref, const Char * data, UInt32 n) {
    return static_cast<ABuffer *>(ref)->writeBytes(data, n);
}

ABE_EXPORT_C UInt32 BufferObjectWriteBytes(BufferObjectRef ref, BufferObjectRef data, UInt32 n) {
    return static_cast<ABuffer *>(ref)->writeBytes(static_cast<ABuffer*>(data), n);
}

ABE_EXPORT_C void BufferObjectFlushBytes(BufferObjectRef ref) {
    static_cast<ABuffer *>(ref)->flushBytes();
}

ABE_EXPORT_C void BufferObjectClearBytes(BufferObjectRef ref) {
    static_cast<ABuffer *>(ref)->clearBytes();
}

ABE_EXPORT_C MessageObjectRef MessageObjectCreate() {
    sp<Message> message = new Message;
    return (MessageObjectRef)message->RetainObject();
}

ABE_EXPORT_C MessageObjectRef MessageObjectCopy(const MessageObjectRef ref) {
    sp<Message> copy = static_cast<const Message *>(ref)->copy();
    return (MessageObjectRef)copy->RetainObject();
}

ABE_EXPORT_C UInt32 MessageObjectGetCount(const MessageObjectRef ref) {
    return static_cast<const Message *>(ref)->size();
}

ABE_EXPORT_C Bool MessageObjectContains(const MessageObjectRef ref, UInt32 name) {
    return static_cast<const Message *>(ref)->contains(name);
}

ABE_EXPORT_C Bool MessageObjectRemove(MessageObjectRef ref, UInt32 name) {
    return static_cast<Message *>(ref)->remove(name);
}

ABE_EXPORT_C void MessageObjectClear(MessageObjectRef ref) {
    static_cast<Message *>(ref)->clear();
}

#define MessageObjectPut(SUFFIX, DATA_TYPE)                                                     \
    ABE_EXPORT_C void MessageObjectPut##SUFFIX(MessageObjectRef ref, UInt32 name, DATA_TYPE data) {    \
        static_cast<Message *>(ref)->set##SUFFIX(name, data);                                   \
    }

MessageObjectPut(Int32,     Int32);
MessageObjectPut(Int64,     Int64);
MessageObjectPut(Float,     Float32);
MessageObjectPut(Double,    Float64);
MessageObjectPut(Pointer,   void *);
MessageObjectPut(String,    const Char *);

ABE_EXPORT_C void MessageObjectPutObject(MessageObjectRef ref, UInt32 name, SharedObjectRef obj) {
    sp<Message> message = static_cast<Message *>(ref);
    message->setObject(name, static_cast<SharedObject *>(obj));
}

#define MessageObjectGet(SUFFIX, DATA_TYPE)                                                             \
    ABE_EXPORT_C DATA_TYPE MessageObjectGet##SUFFIX(const MessageObjectRef ref, UInt32 name, DATA_TYPE def) {  \
        return static_cast<const Message *>(ref)->find##SUFFIX(name, def);                              \
    }

MessageObjectGet(Int32,     Int32);
MessageObjectGet(Int64,     Int64);
MessageObjectGet(Float,     Float32);
MessageObjectGet(Double,    Float64);
MessageObjectGet(Pointer,   void *);
MessageObjectGet(String,    const Char *);

ABE_EXPORT_C SharedObjectRef MessageObjectGetObject(const MessageObjectRef ref, UInt32 name, SharedObjectRef def) {
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

ABE_EXPORT_C JobObjectRef  JobObjectCreate(UserCallback callback, void * user) {
    sp<Job> runnable = new UserJob(callback, user);
    return (Job *)runnable->RetainObject();
}

ABE_EXPORT_C JobObjectRef  JobObjectCreateWithLooper(LooperObjectRef lp, UserCallback callback, void * user) {
    sp<Job> runnable = new UserJob(callback, user);
    return (Job *)runnable->RetainObject();
}

ABE_EXPORT_C void JobObjectDispatch(JobObjectRef ref, UInt64 after) {
    static_cast<Job *>(ref)->dispatch(after);
}

ABE_EXPORT_C Bool JobObjectSync(JobObjectRef ref, UInt64 deadline) {
    return static_cast<Job *>(ref)->sync(deadline);
}

ABE_EXPORT_C void JobObjectCancel(JobObjectRef ref) {
    static_cast<Job *>(ref)->cancel();
}

ABE_EXPORT_C LooperObjectRef LooperObjectMain() {
    return Looper::Main()->RetainObject();
}

ABE_EXPORT_C LooperObjectRef LooperObjectCurrent() {
    return Looper::Current()->RetainObject();
}

ABE_EXPORT_C LooperObjectRef LooperObjectCreate(const Char * name) {
    return (new Looper(name))->RetainObject();
}

ABE_EXPORT_C void LooperObjectDispatch(LooperObjectRef ref, JobObjectRef job, UInt64 after) {
    static_cast<Looper *>(ref)->dispatch(static_cast<Job *>(job), after);
}

ABE_EXPORT_C Bool LooperObjectSync(LooperObjectRef ref, JobObjectRef job, UInt64 deadline) {
    return static_cast<Looper *>(ref)->sync(static_cast<Job *>(job), deadline);
}

ABE_EXPORT_C Bool LooperObjectRemove(LooperObjectRef ref, JobObjectRef job) {
    return static_cast<Looper *>(ref)->remove(static_cast<Job *>(job));
}

ABE_EXPORT_C Bool LooperObjectFind(LooperObjectRef ref, JobObjectRef job) {
    return static_cast<Looper *>(ref)->exists(static_cast<Job *>(job));
}

ABE_EXPORT_C void SharedLooperFlush(LooperObjectRef ref) {
    static_cast<Looper *>(ref)->flush();
}

ABE_EXPORT_C DispatchQueueRef DispatchQueueCreate(LooperObjectRef ref) {
    return (new DispatchQueue(static_cast<Looper *>(ref)))->RetainObject();
}

ABE_EXPORT_C void DispatchQueueDispatch(DispatchQueueRef ref, JobObjectRef job, UInt64 after) {
    static_cast<DispatchQueue *>(ref)->dispatch(static_cast<Job *>(job), after);
}

ABE_EXPORT_C Bool DispatchQueueSync(DispatchQueueRef ref, JobObjectRef job, UInt64 deadline) {
    return static_cast<DispatchQueue *>(ref)->sync(static_cast<Job *>(job), deadline);
}

ABE_EXPORT_C Bool DispatchQueueRemove(DispatchQueueRef ref, JobObjectRef job) {
    return static_cast<DispatchQueue *>(ref)->remove(static_cast<Job *>(job));
}

ABE_EXPORT_C Bool DispatchQueueFind(DispatchQueueRef ref, JobObjectRef job) {
    return static_cast<DispatchQueue *>(ref)->exists(static_cast<Job *>(job));
}

ABE_EXPORT_C void DispatchQueueFlush(DispatchQueueRef ref) {
    static_cast<DispatchQueue *>(ref)->flush();
}

__END_NAMESPACE_ABE
