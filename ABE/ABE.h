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


#ifndef _TOOLKIT_HEADERS_ALL_H
#define _TOOLKIT_HEADERS_ALL_H

// basic [c & c++]
#include <ABE/basic/Version.h>
#include <ABE/basic/Types.h>
#include <ABE/basic/Atomic.h>

#ifdef LOG_TAG
#include <ABE/basic/Log.h>
#endif

#include <ABE/basic/Hardware.h>
#include <ABE/basic/Time.h>

#ifdef __cplusplus  // only available for c++
#include <ABE/basic/SharedObject.h>
#define sp  Object  // for api compatible, keep for sometime

#include <ABE/basic/Allocator.h>
#include <ABE/basic/SharedBuffer.h>
#include <ABE/basic/String.h>
#include <ABE/basic/Mutex.h>
#include <ABE/basic/Thread.h>

// object types [SharedObject] [c & c++]
#include <ABE/object/Buffer.h>
#include <ABE/object/Message.h>
#include <ABE/object/Content.h>
#include <ABE/object/Runnable.h>
#include <ABE/object/Looper.h>
#include <ABE/object/Event.h>

// tools [non-SharedObject]
#include <ABE/tools/Bits.h>

// containers [non-SharedObject]
#include <ABE/stl/TypeHelper.h>
#include <ABE/stl/List.h>
#include <ABE/stl/Vector.h>
#include <ABE/stl/HashTable.h>
#include <ABE/stl/Queue.h>

#endif //  __cplusplus

// c bindings
__BEGIN_DECLS
typedef void *                  SharedObjectRef;
#define NIL                     (SharedObjectRef)0

__ABE_EXPORT SharedObjectRef    SharedObjectRetain(SharedObjectRef);
__ABE_EXPORT void               SharedObjectRelease(SharedObjectRef);
__ABE_EXPORT size_t             SharedObjectGetRetainCount(SharedObjectRef);
#define SharedObjectIsShared(s)     (SharedObjectGetRetainCount(s) > 1)
#define SharedObjectIsNotShared(s)  !SharedObjectIsShared(s)
__ABE_EXPORT uint32_t           SharedObjectGetID(SharedObjectRef);


typedef SharedObjectRef         AllocatorRef;
__ABE_EXPORT AllocatorRef       AllocatorGetDefault(void);
__ABE_EXPORT AllocatorRef       AllocatorGetDefaultAligned(size_t);
__ABE_EXPORT size_t             AllocatorGetAlignment(AllocatorRef);
__ABE_EXPORT void *             AllocatorAllocate(AllocatorRef, size_t);
__ABE_EXPORT void *             AllocatorReallocate(AllocatorRef, void *, size_t);
__ABE_EXPORT void               AllocatorDeallocate(AllocatorRef, void *);


typedef SharedObjectRef         SharedBufferRef;
__ABE_EXPORT SharedBufferRef    SharedBufferCreate(AllocatorRef allocator, size_t);
__ABE_EXPORT void               SharedBufferRelease(SharedBufferRef);
__ABE_EXPORT char *             SharedBufferGetData(const SharedBufferRef);
__ABE_EXPORT size_t             SharedBufferGetLength(const SharedBufferRef);
__ABE_EXPORT SharedBufferRef    SharedBufferEdit(SharedBufferRef);
__ABE_EXPORT SharedBufferRef    SharedBufferEditWithSize(SharedBufferRef, size_t);
__ABE_EXPORT size_t             SharedBufferReleaseWithoutDeallocate(SharedBufferRef);
__ABE_EXPORT void               SharedBufferDeallocate(SharedBufferRef);


typedef SharedObjectRef         BufferObjectRef;
__ABE_EXPORT BufferObjectRef    BufferObjectCreate(size_t);
__ABE_EXPORT const char *       BufferObjectGetData(const BufferObjectRef);
__ABE_EXPORT size_t             BufferObjectGetCapacity(const BufferObjectRef);
__ABE_EXPORT size_t             BufferObjectGetLength(const BufferObjectRef);
__ABE_EXPORT size_t             BufferObjectGetEmptyLength(const BufferObjectRef);
__ABE_EXPORT size_t             BufferObjectPutData(BufferObjectRef, const char *, size_t);


typedef SharedObjectRef         MessageObjectRef;
__ABE_EXPORT MessageObjectRef   MessageObjectCreate();
__ABE_EXPORT MessageObjectRef   MessageObjectCreateWithId(uint32_t id);
__ABE_EXPORT MessageObjectRef   MessageObjectCopy(MessageObjectRef);
__ABE_EXPORT uint32_t           MessageObjectGetId      (const MessageObjectRef);
__ABE_EXPORT size_t             MessageObjectGetSize    (const MessageObjectRef);
__ABE_EXPORT bool               MessageObjectContains   (const MessageObjectRef, const char *);
__ABE_EXPORT bool               MessageObjectRemove     (MessageObjectRef, const char *);
__ABE_EXPORT void               MessageObjectClear      (MessageObjectRef);
__ABE_EXPORT void               MessageObjectPutInt32   (MessageObjectRef, const char *, int32_t);
__ABE_EXPORT void               MessageObjectPutInt64   (MessageObjectRef, const char *, int64_t);
__ABE_EXPORT void               MessageObjectPutFloat   (MessageObjectRef, const char *, float);
__ABE_EXPORT void               MessageObjectPutDouble  (MessageObjectRef, const char *, double);
__ABE_EXPORT void               MessageObjectPutPointer (MessageObjectRef, const char *, void *);
__ABE_EXPORT void               MessageObjectPutString  (MessageObjectRef, const char *, const char *);
__ABE_EXPORT void               MessageObjectPutObject  (MessageObjectRef, const char *, SharedObjectRef);
__ABE_EXPORT int32_t            MessageObjectGetInt32   (const MessageObjectRef, const char *, int32_t);
__ABE_EXPORT int64_t            MessageObjectGetInt64   (const MessageObjectRef, const char *, int64_t);
__ABE_EXPORT float              MessageObjectGetFloat   (const MessageObjectRef, const char *, float);
__ABE_EXPORT double             MessageObjectGetDouble  (const MessageObjectRef, const char *, double);
__ABE_EXPORT void *             MessageObjectGetPointer (const MessageObjectRef, const char *, void *);
__ABE_EXPORT const char *       MessageObjectGetString  (const MessageObjectRef, const char *, const char *);
__ABE_EXPORT SharedObjectRef    MessageObjectGetObject  (const MessageObjectRef, const char *, SharedObjectRef);


typedef SharedObjectRef         ContentObjectRef;
__ABE_EXPORT ContentObjectRef   ContentObjectCreate(const char *);
__ABE_EXPORT size_t             ContentObjectLength(ContentObjectRef);
__ABE_EXPORT BufferObjectRef    ContentObjectRead(ContentObjectRef, size_t);
__ABE_EXPORT void               ContentObjectReset(ContentObjectRef);


typedef SharedObjectRef         LooperObjectRef;
typedef SharedObjectRef         RunnableObjectRef;
LooperObjectRef                 LooperObjectCreate(const char * name);
__ABE_EXPORT void               LooperObjectLoop(LooperObjectRef);
__ABE_EXPORT void               LooperObjectTerminate(LooperObjectRef);
__ABE_EXPORT void               LooperObjectTerminateAndWait(LooperObjectRef);
__ABE_EXPORT void               LooperObjectPostRunnable(LooperObjectRef, RunnableObjectRef);
__ABE_EXPORT void               LooperObjectPostRunnableWithDelay(LooperObjectRef, RunnableObjectRef, int64_t);
__ABE_EXPORT void               LooperObjectRemoveRunnable(LooperObjectRef, RunnableObjectRef);
__ABE_EXPORT bool               LooperObjectFindRunnable(LooperObjectRef, RunnableObjectRef);
__ABE_EXPORT void               LooperObjectFlush(LooperObjectRef);

__END_DECLS

#endif // _TOOLKIT_HEADERS_ALL_H
