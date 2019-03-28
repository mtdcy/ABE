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


// File:    Message.h
// Author:  mtdcy.chen
// Changes: 
//          1. 20160701     initial version
//

#ifndef _TOOLKIT_HEADERS_MESSAGE_H
#define _TOOLKIT_HEADERS_MESSAGE_H 

#include <ABE/basic/Types.h>
#include <ABE/basic/String.h>
#include <ABE/stl/HashTable.h>

#define MESSAGE_WITH_STL 1

#ifdef __cplusplus
__BEGIN_NAMESPACE_ABE
class Message : public SharedObject {
    public:
        Message(uint32_t what = 0);
        virtual ~Message();
        Message(const Message&);
        Message& operator=(const Message&);

    public:
        enum Type {
            kTypeInt32,
            kTypeInt64,
            kTypeFloat,
            kTypeDouble,
            kTypePointer,
            kTypeString,
            kTypeObject,
        };

    public:
        __ALWAYS_INLINE uint32_t    what        () const { return mWhat; }
        __ALWAYS_INLINE size_t      countEntries() const { return mEntries.size(); }

        void            clear       ();
        bool            contains    (const String& name) const;
        bool            remove      (const String& name);
        String          string      () const;
        bool            contains    (const String& name, Type) const;
        String          getEntryNameAt(size_t index, Type *type) const;

    public:
        // basic types
        void            setInt32    (const String& name, int32_t value);                    // kTypeInt32
        void            setInt64    (const String& name, int64_t value);                    // kTypeInt64
        void            setFloat    (const String& name, float value);                      // kTypeFloat
        void            setDouble   (const String& name, double value);                     // kTypeDouble
        void            setPointer  (const String& name, void *value);                      // kTypePointer
        void            setString   (const String& name, const char *s, size_t len = 0);    // kTypeString
        void            setObject   (const String& name, SharedObject * object);            // kTypeObject

        template <class T> __ALWAYS_INLINE void setObject(const String& name, const sp<T>& o)
        { setObject(name, static_cast<SharedObject *>(o.get())); }

        int32_t         findInt32   (const String& name, int32_t def = 0) const;            // kTypeInt32
        int64_t         findInt64   (const String& name, int64_t def = 0) const;            // kTypeInt64
        float           findFloat   (const String& name, float def = 0) const;              // kTypeFloat
        double          findDouble  (const String& name, double def = 0) const;             // kTypeDouble
        void *          findPointer (const String& name, void * def = NULL) const;          // kTypePointer
        const char *    findString  (const String& name, const char * def = NULL) const;    // kTypeString
        SharedObject *  findObject  (const String& name, SharedObject * def = NULL) const;  // kTypeObject

        // alias
        __ALWAYS_INLINE void setString(const String& name, const String &s)
        { setString(name, s.c_str()); }

    private:
        uint32_t                    mWhat;
        struct Entry {
            Type                    mType;
            union {
                int32_t             i32;
                int64_t             i64;
                float               flt;
                double              dbl;
                void *              ptr;
                SharedObject *      obj;
            } u;
        };

        HashTable<String, Entry>    mEntries;

#if MESSAGE_WITH_STL
    public:
        template <class TYPE> struct ObjectWrapper : public SharedObject {
            TYPE    mValue;
            __ALWAYS_INLINE ObjectWrapper(const TYPE& value) : SharedObject(), mValue(value) { }
            __ALWAYS_INLINE virtual ~ObjectWrapper() { }
        };

        template <class TYPE> __ALWAYS_INLINE void set(const String& name, const TYPE& value)
        { setObject(name, new ObjectWrapper<TYPE>(value)); }

        template <class TYPE> __ALWAYS_INLINE const TYPE& find(const String& name) const
        { return (static_cast<ObjectWrapper<TYPE> *>(findObject(name)))->mValue; }
#endif
};

__END_NAMESPACE_ABE
#endif   // __cplusplus

#ifdef __cplusplus
using __NAMESPACE_ABE::Message;
#else
typedef struct Message Message;
#endif

__BEGIN_DECLS

Message *           SharedMessageCreate();
Message *           SharedMessageCreateWithId(uint32_t id);
Message *           SharedMessageCopy(Message *);
#define SharedMessageRetain(x)          (Message *)SharedObjectRetain((SharedObject *)x)
#define SharedMessageRelease(x)         SharedObjectRelease((SharedObject *)x)
#define SharedMessageGetRetainCount(x)  SharedObjectGetRetainCount((SharedObject *)x)

uint32_t            SharedMessageGetId      (Message *);
size_t              SharedMessageGetCount   (Message *);
bool                SharedMessageContains   (Message *, const char *);
bool                SharedMessageRemove     (Message *, const char *);
void                SharedMessageClear      (Message *);

void                SharedMessagePutInt32   (Message *, const char *, int32_t);
void                SharedMessagePutInt64   (Message *, const char *, int64_t);
void                SharedMessagePutFloat   (Message *, const char *, float);
void                SharedMessagePutDouble  (Message *, const char *, double);
void                SharedMessagePutPointer (Message *, const char *, void *);
void                SharedMessagePutString  (Message *, const char *, const char *);
void                SharedMessagePutObject  (Message *, const char *, SharedObject *);

int32_t             SharedMessageGetInt32   (Message *, const char *, int32_t);
int64_t             SharedMessageGetInt64   (Message *, const char *, int64_t);
float               SharedMessageGetFloat   (Message *, const char *, float);
double              SharedMessageGetDouble  (Message *, const char *, double);
void *              SharedMessageGetPointer (Message *, const char *, void *);
const char *        SharedMessageGetString  (Message *, const char *, const char *);
SharedObject *      SharedMessageGetObject  (Message *, const char *, SharedObject *);

__END_DECLS

#endif  // _TOOLKIT_HEADERS_MESSAGE_H

