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

__BEGIN_DECLS

SharedObjectRef SharedObjectRetain(SharedObjectRef ref) {
    return static_cast<SharedObject *>(ref)->RetainObject();
}

void SharedObjectRelease(SharedObjectRef ref) {
    static_cast<SharedObject *>(ref)->ReleaseObject();
}

size_t SharedObjectGetRetainCount(const SharedObjectRef ref) {
    return static_cast<const SharedObject *>(ref)->GetRetainCount();
}

uint32_t SharedObjectGetID(const SharedObjectRef ref) {
    return static_cast<const SharedObject *>(ref)->GetObjectID();
}

AllocatorRef AllocatorGetDefault(void) {
    return (AllocatorRef)kAllocatorDefault->RetainObject();
}

AllocatorRef AllocatorGetDefaultAligned(size_t alignment) {
    return (AllocatorRef)GetAlignedAllocator(alignment)->RetainObject();
}

void * AllocatorAllocate(AllocatorRef ref, size_t n) {
    return static_cast<Allocator *>(ref)->allocate(n);
}

void * AllocatorReallocate(AllocatorRef ref, void * p, size_t n) {
    return static_cast<Allocator *>(ref)->reallocate(p, n);
}

void AllocatorDeallocate(AllocatorRef ref, void * p) {
    static_cast<Allocator *>(ref)->deallocate(p);
}

SharedBufferRef SharedBufferCreate(AllocatorRef _allocator, size_t sz) {
    return __NAMESPACE_ABE::SharedBuffer::Create(_allocator, sz);
}

void SharedBufferRelease(SharedBufferRef ref) {
    static_cast<SharedBuffer *>(ref)->ReleaseBuffer(false);
}

char * SharedBufferGetDataPointer(SharedBufferRef ref) {
    return static_cast<SharedBuffer *>(ref)->data();
}

const char * SharedBufferGetConstDataPointer(const SharedBufferRef ref) {
    return static_cast<const SharedBuffer *>(ref)->data();
}

size_t SharedBufferGetDataLength(const SharedBufferRef ref) {
    return static_cast<SharedBuffer *>(ref)->size();
}

SharedBufferRef SharedBufferEdit(SharedBufferRef ref) {
    return static_cast<SharedBuffer *>(ref)->edit();
}

SharedBufferRef SharedBufferEditWithSize(SharedBufferRef ref, size_t sz) {
    return static_cast<SharedBuffer *>(ref)->edit(sz);
}

size_t SharedBufferReleaseWithoutDeallocate(SharedBufferRef ref) {
    return static_cast<SharedBuffer *>(ref)->ReleaseBuffer(true);
}

void SharedBufferDeallocate(SharedBufferRef ref) {
    static_cast<SharedBuffer *>(ref)->deallocate();
}

BufferObjectRef BufferObjectCreate(size_t cap) {
    sp<Buffer> buffer = new Buffer(cap);
    return buffer->RetainObject();
}

char * BufferObjectGetDataPointer(BufferObjectRef ref) {
    return static_cast<Buffer *>(ref)->data();
}

const char * BufferObjectGetConstDataPointer(const BufferObjectRef ref) {
    return static_cast<const Buffer *>(ref)->data();
}

size_t BufferObjectGetCapacity(const BufferObjectRef ref) {
    return static_cast<const Buffer *>(ref)->capacity();
}

size_t BufferObjectGetDataLength(const BufferObjectRef ref) {
    return static_cast<const Buffer *>(ref)->size();
}

size_t BufferObjectGetEmptyLength(const BufferObjectRef ref) {
    return static_cast<const Buffer *>(ref)->empty();
}

size_t BufferObjectPutData(BufferObjectRef ref, const char * data, size_t n) {
    return static_cast<Buffer *>(ref)->write(data, n);
}

MessageObjectRef MessageObjectCreate() {
    sp<Message> message = new Message;
    return (MessageObjectRef)message->RetainObject();
}

MessageObjectRef MessageObjectCreateWithId(uint32_t id) {
    sp<Message> message = new Message(id);
    return (MessageObjectRef)message->RetainObject();
}

MessageObjectRef MessageObjectCopy(const MessageObjectRef ref) {
    sp<Message> copy = static_cast<const Message *>(ref)->dup();
    return (MessageObjectRef)copy->RetainObject();
}

uint32_t MessageObjectGetId(const MessageObjectRef ref) {
    return static_cast<const Message *>(ref)->what();
}

size_t MessageObjectGetCount(const MessageObjectRef ref) {
    return static_cast<const Message *>(ref)->countEntries();
}

bool MessageObjectContains(const MessageObjectRef ref, const char * name) {
    return static_cast<const Message *>(ref)->contains(name);
}

bool MessageObjectRemove(MessageObjectRef ref, const char * name) {
    return static_cast<Message *>(ref)->remove(name);
}

void MessageObjectClear(MessageObjectRef ref) {
    static_cast<Message *>(ref)->clear();
}

#define MessageObjectPut(SUFFIX, DATA_TYPE)                                                     \
    void MessageObjectPut##SUFFIX(MessageObjectRef ref, const char * name, DATA_TYPE data) {    \
        static_cast<Message *>(ref)->set##SUFFIX(name, data);                                   \
    }

MessageObjectPut(Int32,     int32_t);
MessageObjectPut(Int64,     int64_t);
MessageObjectPut(Float,     float);
MessageObjectPut(Double,    double);
MessageObjectPut(Pointer,   void *);
MessageObjectPut(String,    const char *);

void MessageObjectPutObject(MessageObjectRef ref, const char * name, SharedObjectRef obj) {
    sp<Message> message = ref;
    message->setObject(name, static_cast<SharedObject *>(obj));
}

#define MessageObjectGet(SUFFIX, DATA_TYPE)                                                             \
    DATA_TYPE MessageObjectGet##SUFFIX(const MessageObjectRef ref, const char * name, DATA_TYPE def) {  \
        return static_cast<const Message *>(ref)->find##SUFFIX(name, def);                              \
    }

MessageObjectGet(Int32,     int32_t);
MessageObjectGet(Int64,     int64_t);
MessageObjectGet(Float,     float);
MessageObjectGet(Double,    double);
MessageObjectGet(Pointer,   void *);
MessageObjectGet(String,    const char *);

SharedObjectRef MessageObjectGetObject(const MessageObjectRef ref, const char * name, SharedObjectRef def) {
    return (SharedObjectRef)static_cast<const Message *>(ref)->findObject(name, static_cast<SharedObject *>(def));
}

ContentObjectRef ContentObjectCreate(const char * url) {
    sp<Content> content = Content::Create(String(url));
    if (content == NULL) return NULL;
    return (ContentObjectRef)content->RetainObject();
}

size_t ContentObjectLength(const ContentObjectRef ref) {
    return static_cast<const Content *>(ref)->length();
}

BufferObjectRef ContentObjectRead(ContentObjectRef ref, size_t size) {
    sp<Buffer> buffer = static_cast<Content *>(ref)->read(size);
    return (BufferObjectRef)buffer->RetainObject();
}

BufferObjectRef ContentObjectReadPosition(ContentObjectRef ref, size_t size, int64_t offset) {
    static_cast<Content *>(ref)->seek(offset);
    sp<Buffer> buffer = static_cast<Content *>(ref)->read(size);
    return (BufferObjectRef)buffer->RetainObject();
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

size_t JobObjectRun(JobObjectRef ref) {
    return static_cast<Job *>(ref)->run();
}

void JobObjectCancel(JobObjectRef ref) {
    static_cast<Job *>(ref)->cancel();
}

LooperObjectRef LooperObjectCreate(const char * name) {
    sp<Looper> looper = new Looper(name);
    return (LooperObjectRef)looper->RetainObject();
}

void LooperObjectPostJob(LooperObjectRef ref, JobObjectRef job) {
    static_cast<Looper *>(ref)->post(job);
}

void LooperObjectPostJobWithDelay(LooperObjectRef ref, JobObjectRef job, int64_t delay) {
    static_cast<Looper *>(ref)->post(job, delay);
}

void LooperObjectRemoveJob(LooperObjectRef ref, JobObjectRef job) {
    static_cast<Looper *>(ref)->remove(job);
}

bool LooperObjectFindJob(LooperObjectRef ref, JobObjectRef job) {
    return static_cast<Looper *>(ref)->exists(job);
}

void SharedLooperFlush(LooperObjectRef ref) {
    static_cast<Looper *>(ref)->flush();
}

__END_DECLS
