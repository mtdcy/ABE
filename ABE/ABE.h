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


#ifndef ABE_HEADERS_ALL_H
#define ABE_HEADERS_ALL_H

// core [c & c++]
#include <ABE/core/Version.h>
#include <ABE/core/Types.h>

#ifdef LOG_TAG
#include <ABE/core/Log.h>
#endif

#include <ABE/core/Hardware.h>
#include <ABE/core/Time.h>

#ifdef __cplusplus  // only available for c++
#define sp  Object  // for api compatible, keep for sometime
#include <ABE/core/Allocator.h>
#include <ABE/core/SharedBuffer.h>
#include <ABE/core/String.h>
#include <ABE/core/Mutex.h>

// object types [SharedObject] [c & c++]
#include <ABE/core/Buffer.h>
#include <ABE/core/Message.h>
#include <ABE/core/Content.h>
#include <ABE/core/Looper.h>

// tools [non-SharedObject]
#include <ABE/tools/Bits.h>

// containers [non-SharedObject]
#include <ABE/stl/TypeHelper.h>
#include <ABE/stl/List.h>
#include <ABE/stl/Vector.h>
#include <ABE/stl/HashTable.h>
#include <ABE/stl/Queue.h>

// math [non-SharedObject]
#include <ABE/math/Matrix.h>

#endif //  __cplusplus

// c bindings
__BEGIN_DECLS

typedef void (*UserCallback)(void *user);

typedef void *                  SharedObjectRef;
#define NIL                     (SharedObjectRef)0

ABE_EXPORT SharedObjectRef      SharedObjectRetain(SharedObjectRef);
ABE_EXPORT void                 SharedObjectRelease(SharedObjectRef);
ABE_EXPORT size_t               SharedObjectGetRetainCount(const SharedObjectRef);
#define SharedObjectIsShared(s)     (SharedObjectGetRetainCount(s) > 1)
#define SharedObjectIsNotShared(s)  !SharedObjectIsShared(s)
ABE_EXPORT uint32_t             SharedObjectGetID(const SharedObjectRef);


typedef SharedObjectRef         AllocatorRef;
ABE_EXPORT AllocatorRef         AllocatorGetDefault(void);
ABE_EXPORT AllocatorRef         AllocatorGetDefaultAligned(size_t);
ABE_EXPORT size_t               AllocatorGetAlignment(AllocatorRef);
ABE_EXPORT void *               AllocatorAllocate(AllocatorRef, size_t);
ABE_EXPORT void *               AllocatorReallocate(AllocatorRef, void *, size_t);
ABE_EXPORT void                 AllocatorDeallocate(AllocatorRef, void *);


typedef SharedObjectRef         SharedBufferRef;
ABE_EXPORT SharedBufferRef      SharedBufferCreate(AllocatorRef allocator, size_t);
ABE_EXPORT void                 SharedBufferRelease(SharedBufferRef);
ABE_EXPORT char *               SharedBufferGetDataPointer(SharedBufferRef);
ABE_EXPORT const char *         SharedBufferGetConstDataPointer(const SharedBufferRef);
ABE_EXPORT size_t               SharedBufferGetDataLength(const SharedBufferRef);
ABE_EXPORT SharedBufferRef      SharedBufferEdit(SharedBufferRef);
ABE_EXPORT SharedBufferRef      SharedBufferEditWithSize(SharedBufferRef, size_t);
ABE_EXPORT size_t               SharedBufferReleaseWithoutDeallocate(SharedBufferRef);
ABE_EXPORT void                 SharedBufferDeallocate(SharedBufferRef);


typedef SharedObjectRef         BufferObjectRef;
ABE_EXPORT BufferObjectRef      BufferObjectCreate(size_t);
ABE_EXPORT char *               BufferObjectGetDataPointer(BufferObjectRef);
ABE_EXPORT const char *         BufferObjectGetConstDataPointer(const BufferObjectRef);
ABE_EXPORT size_t               BufferObjectGetCapacity(const BufferObjectRef);
ABE_EXPORT size_t               BufferObjectGetDataLength(const BufferObjectRef);
ABE_EXPORT size_t               BufferObjectGetEmptyLength(const BufferObjectRef);
ABE_EXPORT size_t               BufferObjectPutData(BufferObjectRef, const char *, size_t);


typedef SharedObjectRef         MessageObjectRef;
ABE_EXPORT MessageObjectRef     MessageObjectCreate();
ABE_EXPORT MessageObjectRef     MessageObjectCreateWithId(uint32_t id);
ABE_EXPORT MessageObjectRef     MessageObjectCopy(MessageObjectRef);
ABE_EXPORT uint32_t             MessageObjectGetId      (const MessageObjectRef);
ABE_EXPORT size_t               MessageObjectGetSize    (const MessageObjectRef);
ABE_EXPORT bool                 MessageObjectContains   (const MessageObjectRef, const char *);
ABE_EXPORT bool                 MessageObjectRemove     (MessageObjectRef, const char *);
ABE_EXPORT void                 MessageObjectClear      (MessageObjectRef);
ABE_EXPORT void                 MessageObjectPutInt32   (MessageObjectRef, const char *, int32_t);
ABE_EXPORT void                 MessageObjectPutInt64   (MessageObjectRef, const char *, int64_t);
ABE_EXPORT void                 MessageObjectPutFloat   (MessageObjectRef, const char *, float);
ABE_EXPORT void                 MessageObjectPutDouble  (MessageObjectRef, const char *, double);
ABE_EXPORT void                 MessageObjectPutPointer (MessageObjectRef, const char *, void *);
ABE_EXPORT void                 MessageObjectPutString  (MessageObjectRef, const char *, const char *);
ABE_EXPORT void                 MessageObjectPutObject  (MessageObjectRef, const char *, SharedObjectRef);
ABE_EXPORT int32_t              MessageObjectGetInt32   (const MessageObjectRef, const char *, int32_t);
ABE_EXPORT int64_t              MessageObjectGetInt64   (const MessageObjectRef, const char *, int64_t);
ABE_EXPORT float                MessageObjectGetFloat   (const MessageObjectRef, const char *, float);
ABE_EXPORT double               MessageObjectGetDouble  (const MessageObjectRef, const char *, double);
ABE_EXPORT void *               MessageObjectGetPointer (const MessageObjectRef, const char *, void *);
ABE_EXPORT const char *         MessageObjectGetString  (const MessageObjectRef, const char *, const char *);
ABE_EXPORT SharedObjectRef      MessageObjectGetObject  (const MessageObjectRef, const char *, SharedObjectRef);


typedef SharedObjectRef         ContentObjectRef;
ABE_EXPORT ContentObjectRef     ContentObjectCreate(const char *);
ABE_EXPORT size_t               ContentObjectLength(ContentObjectRef);
ABE_EXPORT BufferObjectRef      ContentObjectRead(ContentObjectRef, size_t);
ABE_EXPORT BufferObjectRef      ContentObjectReadPosition(ContentObjectRef, size_t, int64_t);

typedef SharedObjectRef         JobObjectRef;
typedef SharedObjectRef         LooperObjectRef;

ABE_EXPORT JobObjectRef         JobObjectCreate(UserCallback, void *);
ABE_EXPORT JobObjectRef         JobObjectCreateWithLooper(LooperObjectRef, UserCallback, void *);
ABE_EXPORT size_t               JobObjectRun(JobObjectRef);
ABE_EXPORT void                 JobObjectCancel(JobObjectRef);

ABE_EXPORT LooperObjectRef      LooperObjectCreate(const char * name);
ABE_EXPORT void                 LooperObjectPostJob(LooperObjectRef, JobObjectRef);
ABE_EXPORT void                 LooperObjectPostJobWithDelay(LooperObjectRef, JobObjectRef, int64_t);
ABE_EXPORT void                 LooperObjectRemoveJob(LooperObjectRef, JobObjectRef);
ABE_EXPORT bool                 LooperObjectFindJob(LooperObjectRef, JobObjectRef);
ABE_EXPORT void                 LooperObjectFlush(LooperObjectRef);

__END_DECLS

#endif // ABE_HEADERS_ALL_H
