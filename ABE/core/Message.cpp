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

#define LOG_TAG "Message"
#include "ABE/core/Log.h"
#include "Message.h"
#include "ABE/stl/Vector.h"

#include <ctype.h>
#include <stdio.h> 
#include <string.h> 
#include <stdlib.h>

__BEGIN_NAMESPACE_ABE

///////////////////////////////////////////////////////////////////////////
static bool isFourcc(int32_t what) {
    return isprint(what & 0xff)
        && isprint((what >> 8) & 0xff)
        && isprint((what >> 16) & 0xff)
        && isprint((what >> 24) & 0xff);
}

Message::Message(uint32_t what)
    : mWhat(what), mEntries()
{
}

sp<Message> Message::dup() const {
    sp<Message> message = new Message;
    HashTable<uint32_t, Entry>::const_iterator it = mEntries.cbegin();
    for (; it != mEntries.cend(); ++it) {
        const uint32_t name = it.key();
        Entry copy          = it.value(); // copy value
        switch (copy.mType) {
            case kTypeString:
                copy.u.ptr = strdup((const char *)copy.u.ptr);
                break;
            case kTypeObject:
                copy.u.obj->RetainObject();
                break;
            default:
                break;
        }
        message->mEntries.insert(name, copy);
    }
    return message;
}

Message::~Message() {
    clear();
}

void Message::clear() {
    HashTable<uint32_t, Entry>::iterator it = mEntries.begin();
    for (; it != mEntries.end(); ++it) {
        Entry &e = it.value();
        switch (e.mType) {
            case kTypeString:
                free(e.u.ptr);
                break;
            case kTypeObject:
                e.u.obj->RetainObject();
                break;
            case kTypePointer:
            default:
                break;
        }
    }
    mEntries.clear();
}

bool Message::contains(uint32_t name) const {
    return mEntries.find(name) != NULL;
}

bool Message::contains(uint32_t name, Type type) const {
    const Entry *e = mEntries.find(name);
    if (e != NULL && e->mType == type) {
        return true;
    }
    return false;
}

#define BASIC_TYPE(NAME,FIELDNAME,TYPENAME)                                 \
    void Message::set##NAME(uint32_t name, TYPENAME value) {                \
        Entry e;                                                            \
        e.mType         = kType##NAME;                                      \
        e.u.FIELDNAME   = value;                                            \
        if (mEntries.find(name)) {                                          \
            remove(name);                                                   \
        }                                                                   \
        mEntries.insert(name, e);                                           \
    }                                                                       \
    TYPENAME Message::find##NAME(uint32_t name, TYPENAME def) const {       \
        const Entry *e  = mEntries.find(name);                              \
        if (e && e->mType == kType##NAME) {                                 \
            return e->u.FIELDNAME;                                          \
        }                                                                   \
        return def;                                                         \
    }

BASIC_TYPE(Int32,   i32,        int32_t);
BASIC_TYPE(Int64,   i64,        int64_t);
BASIC_TYPE(Float,   flt,        float);
BASIC_TYPE(Double,  dbl,        double);
BASIC_TYPE(Pointer, ptr,        void *);

#undef BASIC_TYPE

#ifdef __MINGW32__
static ABE_INLINE char * strndup(const char *s, size_t len) {
    // plus 1 for NULL terminating
    char * dup = (char *)malloc(len+1);
    memcpy(dup, s, len+1);
    return dup;
}
#endif

void Message::setString(uint32_t name, const char *s, size_t len) {
    if (!len) len = strlen(s);
    Entry e;
    e.mType         = kTypeString;
    e.u.ptr         = strndup(s, len);
    if (mEntries.find(name)) {
        remove(name);
    } 
    mEntries.insert(name, e);
}

const char * Message::findString(uint32_t name, const char *def) const {
    const Entry *e = mEntries.find(name);
    if (e && e->mType == kTypeString) {
        return (const char *)e->u.ptr;
    }
    return def;
}

void Message::setObject(uint32_t name, SharedObject *object) {
    Entry e;
    e.mType         = kTypeObject;
    e.u.obj         = object->RetainObject();
    if (mEntries.find(name))    remove(name);
    mEntries.insert(name, e);
}

SharedObject * Message::findObject(uint32_t name, SharedObject * def) const {
    const Entry * e = mEntries.find(name);
    if (e && e->mType == kTypeObject) {
        return e->u.obj;
    }
    return def;
}

bool Message::remove(uint32_t name) {
    Entry *e = mEntries.find(name);
    if (!e) return false;

    switch (e->mType) {
        case kTypeString: 
            free(e->u.ptr);
            break;
        case kTypeObject:
            e->u.obj->ReleaseObject();
            break;
        default:
            break;
    }
    return mEntries.erase(name);
}

String Message::string() const {
    String s = String::format("Message ");

    String tmp;
    if (isFourcc(mWhat)) {
        tmp = String::format("'%.4s'", (const char *)&mWhat);
    } else {
        tmp = String::format("0x%08x", mWhat);
    }
    s.append(tmp);

    s.append(" = {\n");

    HashTable<uint32_t, Entry>::const_iterator it = mEntries.cbegin();
    for (; it != mEntries.cend(); ++it) {
        const uint32_t name = it.key();
        const Entry& e      = it.value();
        
        switch (e.mType) {
            case kTypeInt32:
                if (isFourcc(e.u.i32))
                    tmp = String::format("int32_t '%.4s' = '%.4s'",
                                         (const char *)&name, (const char*)&e.u.i32);
                else
                    tmp = String::format("int32_t '%.4s' = %d",
                                         (const char *)&name, e.u.i32);
                break;
            case kTypeInt64:
                tmp = String::format("int64_t '%.4s' = %lld",
                                     (const char *)&name, e.u.i64);
                break;
            case kTypeFloat:
                tmp = String::format("float '%.4s' = %f",
                                     (const char *)&name, e.u.flt);
                break;
            case kTypeDouble:
                tmp = String::format("double '%.4s' = %f",
                                     (const char *)&name, e.u.dbl);
                break;
            case kTypePointer:
                tmp = String::format("void * '%.4s' = %p",
                                     (const char *)&name, e.u.ptr);
                break;
            case kTypeString:
                tmp = String::format("string '%.4s' = \"%s\"",
                                     (const char *)&name,
                                     (static_cast<const char*>(e.u.ptr)));
                break;
            case kTypeObject:
                tmp = String::format("object '%.4s' = %p",
                                     (const char *)&name,
                                     e.u.obj);
                break;
            default:
                FATAL("should not be here.");
                break;
        }

        s.append("  ");
        s.append(tmp);
        s.append("\n");
    }

    s.append("}");

    return s;
}

uint32_t Message::getEntryNameAt(size_t index, Type *type) const {
    CHECK_LT(index, countEntries());
    HashTable<uint32_t, Entry>::const_iterator it = mEntries.cbegin();
    for (size_t i = 0; i < index; ++it, ++i) { }
    const Entry& e = it.value();
    *type = e.mType;
    return it.key();
}

__END_NAMESPACE_ABE
