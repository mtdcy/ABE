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
#include "ABE/basic/Log.h"
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

Object<Message> Message::dup() const {
    Object<Message> message = new Message;
    HashTable<String, Entry>::const_iterator it = mEntries.cbegin();
    for (; it != mEntries.cend(); ++it) {
        const String& name  = it.key();
        Entry copy          = it.value(); // copy value
        switch (copy.mType) {
            case kTypeString:
                copy.u.ptr = strdup((const char *)copy.u.ptr);
                break;
            case kTypeObject:
            case kTypeValue:
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
    HashTable<String, Entry>::iterator it = mEntries.begin();
    for (; it != mEntries.end(); ++it) {
        Entry &e = it.value();
        switch (e.mType) {
            case kTypeString:
                free(e.u.ptr);
                break;
            case kTypeObject:
            case kTypeValue:
                e.u.obj->RetainObject();
                break;
            case kTypePointer:
            default:
                break;
        }
    }
    mEntries.clear();
}

bool Message::contains(const String& name) const {
    return mEntries.find(name) != NULL;
}

bool Message::contains(const String &name, Type type) const {
    const Entry *e = mEntries.find(name);
    if (e != NULL && e->mType == type) {
        return true;
    }
    return false;
}

#define BASIC_TYPE(NAME,FIELDNAME,TYPENAME)                                 \
    void Message::set##NAME(const String& name, TYPENAME value) {           \
        Entry e;                                                            \
        e.mType         = kType##NAME;                                      \
        e.u.FIELDNAME   = value;                                            \
        if (mEntries.find(name)) {                                          \
            remove(name);                                                   \
        }                                                                   \
        mEntries.insert(name, e);                                           \
    }                                                                       \
    TYPENAME Message::find##NAME(const String& name, TYPENAME def) const {  \
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

void Message::setString( const String& name, const char *s, size_t len) {
    if (!len) len = strlen(s);
    Entry e;
    e.mType         = kTypeString;
    e.u.ptr         = strndup(s, len);
    if (mEntries.find(name)) {
        remove(name);
    } 
    mEntries.insert(name, e);
}

const char * Message::findString(const String& name, const char *def) const {
    const Entry *e = mEntries.find(name);
    if (e && e->mType == kTypeString) {
        return (const char *)e->u.ptr;
    }
    return def;
}

void Message::setObject(const String &name, SharedObject *object) {
    Entry e;
    e.mType         = kTypeObject;
    e.u.obj         = object->RetainObject();
    if (mEntries.find(name))    remove(name);
    mEntries.insert(name, e);
}

SharedObject * Message::findObject(const String &name, SharedObject * def) const {
    const Entry * e = mEntries.find(name);
    if (e && e->mType == kTypeObject) {
        return e->u.obj;
    }
    return def;
}

void Message::_setValue(const String &name, SharedObject *object) {
    Entry e;
    e.mType         = kTypeValue;
    e.u.obj         = object->RetainObject();
    if (mEntries.find(name))    remove(name);
    mEntries.insert(name, e);
}

SharedObject * Message::_findValue(const String &name) const {
    const Entry * e = mEntries.find(name);
    CHECK_NULL(e);
    CHECK_TRUE(e->mType == kTypeValue);
    return e->u.obj;
}

bool Message::remove(const String& name) {
    Entry *e = mEntries.find(name);
    if (!e) return false;

    switch (e->mType) {
        case kTypeString: 
            free(e->u.ptr);
            break;
        case kTypeObject:
        case kTypeValue:
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
        tmp = String::format(
                "'%c%c%c%c'",
                (char)(mWhat >> 24),
                (char)((mWhat >> 16) & 0xff),
                (char)((mWhat >> 8) & 0xff),
                (char)(mWhat & 0xff));
    } else {
        tmp = String::format("0x%08x", mWhat);
    }
    s.append(tmp);

    s.append(" = {\n");

    HashTable<String, Entry>::const_iterator it = mEntries.cbegin();
    for (; it != mEntries.cend(); ++it) {
        const String& name = it.key();
        const Entry& e = it.value();

        switch (e.mType) {
            case kTypeInt32:
                if (isFourcc(e.u.i32))
                    tmp = String::format("int32_t %s = '%c%c%c%c'",
                            name.c_str(),
                            (char)(e.u.i32 >> 24),
                            (char)((e.u.i32 >> 16) & 0xff),
                            (char)((e.u.i32 >> 8) & 0xff),
                            (char)(e.u.i32 & 0xff));
                else
                    tmp = String::format(
                            "int32_t %s = %d", name.c_str(), e.u.i32);
                break;
            case kTypeInt64:
                tmp = String::format(
                        "int64_t %s = %lld", name.c_str(), e.u.i64);
                break;
            case kTypeFloat:
                tmp = String::format(
                        "float %s = %f", name.c_str(), e.u.flt);
                break;
            case kTypeDouble:
                tmp = String::format(
                        "double %s = %f", name.c_str(), e.u.dbl);
                break;
            case kTypePointer:
                tmp = String::format(
                        "void *%s = %p", name.c_str(), e.u.ptr);
                break;
            case kTypeString:
                tmp = String::format(
                        "string %s = \"%s\"",
                        name.c_str(),
                        (static_cast<const char*>(e.u.ptr)));
                break;
            case kTypeObject:
                tmp = String::format(
                        "object %s = %p[%" PRIx32 "]", name.c_str(),
                        e.u.obj,
                        e.u.obj->GetObjectID());
                break;
            case kTypeValue:
                tmp = String::format(
                         "value %s = %p[%" PRIx32 "]", name.c_str(),
                         e.u.obj,
                         e.u.obj->GetObjectID());
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

String Message::getEntryNameAt(size_t index, Type *type) const {
    CHECK_LT(index, countEntries());
    HashTable<String, Entry>::const_iterator it = mEntries.cbegin();
    for (size_t i = 0; i < index; ++it, ++i) { }
    const String& name = it.key();
    const Entry& e = it.value();
    *type = e.mType;
    return name;
}

__END_NAMESPACE_ABE
