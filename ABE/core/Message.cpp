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

template <> struct is_trivial_copy<Message::Entry> { enum { value = True }; };

///////////////////////////////////////////////////////////////////////////
static Bool isFourcc(UInt32 what) {
    return isprint(what & 0xff)
        && isprint((what >> 8) & 0xff)
        && isprint((what >> 16) & 0xff)
        && isprint((what >> 24) & 0xff);
}

Message::Message() : mEntries() {
}

void Message::onFirstRetain() {
}

void Message::onLastRetain() {
    clear();
}

sp<Message> Message::copy() const {
    sp<Message> message = new Message;
    HashTable<UInt32, Entry>::const_iterator it = mEntries.cbegin();
    for (; it != mEntries.cend(); ++it) {
        const UInt32 name = it.key();
        Entry copy          = it.value(); // copy value
        switch (copy.mType) {
            case kTypeString:
                copy.u.ptr = strdup((const Char *)copy.u.ptr);
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

void Message::clear() {
    HashTable<UInt32, Entry>::iterator it = mEntries.begin();
    for (; it != mEntries.end(); ++it) {
        Entry &e = it.value();
        switch (e.mType) {
            case kTypeString:
                free(e.u.ptr);
                break;
            case kTypeObject:
                e.u.obj->ReleaseObject();
                break;
            case kTypePointer:
            default:
                break;
        }
    }
    mEntries.clear();
}

Bool Message::contains(UInt32 name) const {
    return mEntries.find(name) != Nil;
}

Bool Message::contains(UInt32 name, Type type) const {
    const Entry *e = mEntries.find(name);
    if (e != Nil && e->mType == type) {
        return True;
    }
    return False;
}

#define BASIC_TYPE(NAME,FIELDNAME,TYPENAME)                                 \
    void Message::set##NAME(UInt32 name, TYPENAME value) {                  \
        Entry e;                                                            \
        e.mType         = kType##NAME;                                      \
        e.u.FIELDNAME   = value;                                            \
        if (mEntries.find(name)) {                                          \
            remove(name);                                                   \
        }                                                                   \
        mEntries.insert(name, e);                                           \
    }                                                                       \
    TYPENAME Message::find##NAME(UInt32 name, TYPENAME def) const {         \
        const Entry *e  = mEntries.find(name);                              \
        if (e && e->mType == kType##NAME) {                                 \
            return e->u.FIELDNAME;                                          \
        }                                                                   \
        return def;                                                         \
    }

BASIC_TYPE(Int32,   i32,        Int32);
BASIC_TYPE(Int64,   i64,        Int64);
BASIC_TYPE(Float,   flt,        Float32);
BASIC_TYPE(Double,  dbl,        Float64);
BASIC_TYPE(Pointer, ptr,        void *);

#undef BASIC_TYPE

#ifdef __MINGW32__
static ABE_INLINE Char * strndup(const Char *s, UInt32 len) {
    // plus 1 for Nil terminating
    Char * dup = (Char *)malloc(len+1);
    memcpy(dup, s, len+1);
    return dup;
}
#endif

void Message::setString(UInt32 name, const Char *s, UInt32 len) {
    if (!len) len = strlen(s);
    Entry e;
    e.mType         = kTypeString;
    e.u.ptr         = strndup(s, len);
    if (mEntries.find(name)) {
        remove(name);
    } 
    mEntries.insert(name, e);
}

const Char * Message::findString(UInt32 name, const Char *def) const {
    const Entry *e = mEntries.find(name);
    if (e && e->mType == kTypeString) {
        return (const Char *)e->u.ptr;
    }
    return def;
}

void Message::setObject(UInt32 name, SharedObject *object) {
    Entry e;
    e.mType         = kTypeObject;
    e.u.obj         = object->RetainObject();
    if (mEntries.find(name))    remove(name);
    mEntries.insert(name, e);
}

SharedObject * Message::findObject(UInt32 name, SharedObject * def) const {
    const Entry * e = mEntries.find(name);
    if (e && e->mType == kTypeObject) {
        return e->u.obj;
    }
    return def;
}

Bool Message::remove(UInt32 name) {
    Entry *e = mEntries.find(name);
    if (!e) return False;

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
    String s = String::format("Message = {\n");

    HashTable<UInt32, Entry>::const_iterator it = mEntries.cbegin();
    for (; it != mEntries.cend(); ++it) {
        const UInt32 name = it.key();
        const Entry& e      = it.value();
        
        String tmp;
        switch (e.mType) {
            case kTypeInt32:
                if (isFourcc(e.u.i32))
                    tmp = String::format("Int32 '%.4s' = '%.4s'",
                                         (const Char *)&name, (const Char*)&e.u.i32);
                else
                    tmp = String::format("Int32 '%.4s' = %d",
                                         (const Char *)&name, e.u.i32);
                break;
            case kTypeInt64:
                tmp = String::format("Int64 '%.4s' = %lld",
                                     (const Char *)&name, e.u.i64);
                break;
            case kTypeFloat:
                tmp = String::format("Float32 '%.4s' = %f",
                                     (const Char *)&name, e.u.flt);
                break;
            case kTypeDouble:
                tmp = String::format("Float64 '%.4s' = %f",
                                     (const Char *)&name, e.u.dbl);
                break;
            case kTypePointer:
                tmp = String::format("void * '%.4s' = %p",
                                     (const Char *)&name, e.u.ptr);
                break;
            case kTypeString:
                tmp = String::format("string '%.4s' = \"%s\"",
                                     (const Char *)&name,
                                     (static_cast<const Char*>(e.u.ptr)));
                break;
            case kTypeObject:
                tmp = String::format("object '%.4s' = %p",
                                     (const Char *)&name,
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

UInt32 Message::name(UInt32 index, Type& type) const {
    CHECK_LT(index, size());
    HashTable<UInt32, Entry>::const_iterator it = mEntries.cbegin();
    for (UInt32 i = 0; i < index; ++it, ++i) { }
    const Entry& e = it.value();
    type = e.mType;
    return it.key();
}

__END_NAMESPACE_ABE
