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

char * SharedBufferGetData(const SharedBufferRef ref) {
    return static_cast<SharedBuffer *>(ref)->data();
}

size_t SharedBufferGetLength(const SharedBufferRef ref) {
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
    Object<Buffer> buffer = new Buffer(cap);
    return buffer->RetainObject();
}

const char * BufferObjectGetData(const BufferObjectRef ref) {
    return static_cast<const Buffer *>(ref)->data();
}

size_t BufferObjectGetCapacity(const BufferObjectRef ref) {
    return static_cast<const Buffer *>(ref)->capacity();
}

size_t BufferObjectGetLength(const BufferObjectRef ref) {
    return static_cast<const Buffer *>(ref)->size();
}

size_t BufferObjectGetEmptyLength(const BufferObjectRef ref) {
    return static_cast<const Buffer *>(ref)->empty();
}

size_t BufferObjectPutData(BufferObjectRef ref, const char * data, size_t n) {
    return static_cast<Buffer *>(ref)->write(data, n);
}

MessageObjectRef MessageObjectCreate() {
    Object<Message> message = new Message;
    return (MessageObjectRef)message->RetainObject();
}

MessageObjectRef MessageObjectCreateWithId(uint32_t id) {
    Object<Message> message = new Message(id);
    return (MessageObjectRef)message->RetainObject();
}

MessageObjectRef MessageObjectCopy(const MessageObjectRef ref) {
    Object<Message> copy = static_cast<const Message *>(ref)->dup();
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
    Object<Message> message = ref;
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
    Object<Content> content = Content::Create(url);
    if (content == NULL) return NULL;
    return (ContentObjectRef)content->RetainObject();
}

size_t ContentObjectLength(const ContentObjectRef ref) {
    return static_cast<const Content *>(ref)->size();
}

BufferObjectRef ContentObjectRead(ContentObjectRef ref, size_t size) {
    Object<Buffer> buffer = static_cast<Content *>(ref)->read(size);
    return (BufferObjectRef)buffer->RetainObject();
}

void ContentObjectReset(ContentObjectRef ref) {
    Object<Content> content = ref;
    content->reset();
}

//////////////////////////////////////////////////////////////////////////////////
struct UserRunnable : public Runnable {
    void (*callback)(void *);
    void * user;
    UserRunnable(void (*Callback)(void *), void * User) : Runnable(),
    callback(Callback), user(User) { }
    virtual void run() { callback(user); }
};

RunnableObjectRef  RunnableObjectCreate(void (*Callback)(void *), void * User) {
    Object<Runnable> runnable = new UserRunnable(Callback, User);
    return (Runnable *)runnable->RetainObject();
}

LooperObjectRef SharedLooperCreate(const char * name) {
    Object<Looper> looper = Looper::Create(name);
    return (LooperObjectRef)looper->RetainObject();
}

void SharedLooperLoop(LooperObjectRef ref) {
    static_cast<Looper *>(ref)->loop();
}

void SharedLooperTerminate(LooperObjectRef ref) {
    static_cast<Looper *>(ref)->terminate();
}

void SharedLooperTerminateAndWait(LooperObjectRef ref) {
    static_cast<Looper *>(ref)->terminate(true);
}

void SharedLooperPostRunnable(LooperObjectRef ref, RunnableObjectRef run) {
    static_cast<Looper *>(ref)->post(run);
}

void SharedLooperPostRunnableWithDelay(LooperObjectRef ref, RunnableObjectRef run, int64_t delay) {
    static_cast<Looper *>(ref)->post(run, delay);
}

void SharedLooperRemoveRunnable(LooperObjectRef ref, RunnableObjectRef run) {
    static_cast<Looper *>(ref)->remove(run);
}

bool SharedLooperFindRunnable(LooperObjectRef ref, RunnableObjectRef run) {
    return static_cast<Looper *>(ref)->exists(run);
}

void SharedLooperFlush(LooperObjectRef ref) {
    static_cast<Looper *>(ref)->flush();
}

__END_DECLS
