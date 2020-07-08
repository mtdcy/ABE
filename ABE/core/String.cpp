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
 *    and/or rhs materials provided with the distribution.
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


// File:    String.cpp
// Author:  mtdcy.chen
// Changes: 
//          1. 20160701     initial version
//

#define __STDC_FORMAT_MACROS

#define LOG_TAG   "String"
#include "Log.h"
#include "String.h"

#include "private/ConvertUTF.h"

#include <ctype.h> // isspace 
#include <strings.h> 
#include <string.h> 
#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>

static UInt32 CStringPrintf(void *str, UInt32 size, const Char *format, ...) {
    va_list ap;
    va_start(ap, format);
    UInt32 len = vsnprintf((Char*)str, size, format, ap);
    va_end(ap);
    return len;
}

// about cow 
// http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2008/n2534.html

// Note: 
// 1. no CHECK_XX can be used in String's constructor, will cause loop

__BEGIN_NAMESPACE_ABE

String String::format(const Char *format, ...) {
    va_list ap;
    Char buf[1024];
    va_start(ap, format);

    UInt32 len = vsnprintf(buf, 1024, format, ap);

    va_end(ap);

    String result(buf, len);
    return result;
}

String String::format(const Char *format, va_list ap) {
    Char buf[1024];
    UInt32 len = vsnprintf(buf, 1024, format, ap);
    String result(buf, len);
    return result;
}

String::String() : mData(Nil), mSize(0) { }

/* static */
String String::Null;

String::String(const Char *s, UInt32 n) : mData(Nil), mSize(0)
{
    CHECK_NULL(s);
    if (!n) mSize = strlen(s);
    else    mSize = strnlen(s, n);
    mData = SharedBuffer::Create(kAllocatorDefault, mSize + 1);
    Char * buf = mData->data();
    if (mSize) memcpy(buf, s, mSize);
    buf[mSize] = '\0';
}

String::String(const String &rhs) : mData(Nil), mSize(0)
{
    if (rhs.mData) mData = rhs.mData->RetainBuffer();
    mSize   = rhs.mSize;
}

String String::UTF16(const Char *s, UInt32 n) {
    CHECK_NULL(s);
    CHECK_GT(n, 0);
    
    // is the size right?
    const UInt32 length = n * 4 + 1;
    Char utf8[length];
    Char * buf_start = utf8;
    Char * buf_end = buf_start + length;
    Char * buf = buf_start;

    ConversionResult result = ConvertUTF16toUTF8(
            (const ::UTF16**)&s, (const ::UTF16*)(s + n),
            (UTF8**)&buf,
            (UTF8*)buf_end,
            lenientConversion);
    if (result != conversionOK) {
        ERROR("ConvertUTF16toUTF8 failed, ret = %d.", result);
        return String::Null;
    }

    return String(utf8, buf - buf_start);
}

#define STRING_FROM_NUMBER(TYPE, SIZE, PRI)                                 \
    String::String(const TYPE v) : mData(Nil), mSize(0) {                   \
        mData = SharedBuffer::Create(kAllocatorDefault, SIZE + 1);          \
        Char * buf = mData->data();                                         \
        int result = CStringPrintf(buf, SIZE, "%" PRI, v);                  \
        CHECK_GT(result, 0); CHECK_LE(result, SIZE);                        \
        mSize               = result;                                       \
        buf[mSize]          = '\0';                                         \
    }

STRING_FROM_NUMBER(Char,        2,      "c");
STRING_FROM_NUMBER(UInt8,       4,      PRIu8);
STRING_FROM_NUMBER(Int8,        4,      PRId8);
STRING_FROM_NUMBER(UInt16,      6,      PRIu16);
STRING_FROM_NUMBER(Int16,       6,      PRId16);
STRING_FROM_NUMBER(UInt32,      16,     PRIu32);
STRING_FROM_NUMBER(Int32,       16,     PRId32);
STRING_FROM_NUMBER(UInt64,      32,     PRIu64);
STRING_FROM_NUMBER(Int64,       32,     PRId64);
STRING_FROM_NUMBER(Float32,     32,     "f");
STRING_FROM_NUMBER(Float64,     64,     "f");
STRING_FROM_NUMBER(void *,      32,     "xp");

String::~String() {
    clear();
}

String String::basename() const {
    if (mData == Nil) return String::Null;
    
    Char * buf = mData->data();
    const Char *last = strrchr(buf, '/');
    if (last) {
        const Char *dot = strrchr(last, '.');
        ++last;
        if (dot) {
            return String(last, dot - last);
        } else {
            return String(last);
        }
    } else {
        return *this;
    }
}

String String::dirname() const {
    FATAL("TODO: add implementation");
    return String("");
}

const Char& String::operator[](UInt32 index) const {
    CHECK_NULL(mData);
    CHECK_LE(index, size());
    Char * buf = mData->data();
    return buf[index];
}

Char& String::operator[](UInt32 index) {
    CHECK_NULL(mData);
    CHECK_LE(index, size());
    mData = mData->edit();
    Char * buf = mData->data();
    return buf[index];
}

String& String::set(const String& s) {
    if (mData) mData->ReleaseBuffer();
    mData   = s.mData->RetainBuffer();
    mSize   = s.mSize;
    return *this;
}

String& String::set(const Char * s, UInt32 n) {
    if (!n) n = strlen(s);
    if (mData && mData->size() > n) {
        // no need to release buffer
    } else {
        if (mData) mData->ReleaseBuffer();
        mData = SharedBuffer::Create(kAllocatorDefault, n + 1);
    }
    memcpy(mData->data(), s, n);
    *(mData->data() + n) = '\0';    // null-terminator
    mSize = n;
    return *this;
}

String& String::append(const String& s) {
    if (!s.mSize) return *this; // append empty string ?
    if (!mSize) return set(s);

    UInt32 m = mSize + s.mSize + 1;
    mData = mData->edit(m);

    // plus 1 for '\0'
    Char * buf = mData->data();
    strncpy(buf + mSize, s.mData->data(), s.mSize + 1);
    mSize += s.mSize;
    return *this;
}

String& String::append(const Char * s, UInt32 n) {
    if (!n) n = strlen(s);
    if (!n) return *this;   // append empty string
    if (!mSize) return set(s, n);
    
    UInt32 m = mSize + n + 1;   // plus 1 for '\0'
    mData = mData->edit(m);
    strncpy(mData->data() + mSize, s, n + 1);   // plus 1 for '\0';
    mSize += n;
    return *this;
}

String& String::insert(UInt32 pos, const String& s) {
    CHECK_LE(pos, mSize);
    if (!s.mSize) return *this; // insert an empty string ???

    if (!mSize) return set(s);
    if (pos == mSize) return append(s);

    UInt32 m = mSize + s.mSize + 1;
    mData = mData->edit(m);

    // +1 to move the terminating null byte
    Char * buf = mData->data();
    memmove(buf + pos + s.mSize, buf + pos, mSize - pos + 1);
    strncpy(buf + pos, s.mData->data(), s.mSize);
    mSize   += s.mSize;
    return *this;
}

String& String::insert(UInt32 pos, const Char *s, UInt32 n) {
    CHECK_LE(pos, mSize);
    if (!n) n = strlen(s);
    if (!n) return *this; // insert empty string
    
    if (!mSize) return set(s, n);
    if (pos == mSize) return append(s, n);
    
    UInt32 m = mSize + n + 1;
    mData = mData->edit(m);
    
    Char * buf = mData->data();
    memmove(buf + pos + n, buf + pos, mSize - pos + 1); // plus 1 for '\0'
    strncpy(buf + pos, s, n);
    mSize += n;
    return *this;
}

void String::clear() {
    if (mData == Nil) return;
    mData->ReleaseBuffer();
    mData = Nil;
    mSize = 0;
}

UInt32 String::hash() const {
    if (mData == Nil) return 0;
    UInt32 x = 0;
    const Char *s = mData->data();
    for (UInt32 i = 0; i < mSize; ++i) {
        x = (x * 31) + s[i];
    }
    return x;
}

String& String::trim() {
    if (mSize == 0) return *this;

    mData = mData->edit();

    Char *s = mData->data();
    UInt32 i = 0;
    while (i < mSize && isspace(s[i])) {
        ++i;
    }

    UInt32 j = mSize;
    while (j > i && isspace(s[j - 1])) {
        --j;
    }

    memmove(s, &s[i], j - i);
    mSize = j - i;
    s[mSize] = '\0';
    return *this;
}

String& String::erase(UInt32 pos, UInt32 n) {
    CHECK_TRUE(pos < mSize);
    CHECK_TRUE(pos + n <= mSize);

    mData = mData->edit();
    Char * buf = mData->data();

    if (pos + n < mSize) {
        memmove(buf + pos, buf + pos + n, mSize - (pos + n));
    }
    mSize -= n;
    buf[mSize] = '\0';
    return *this;
}

String& String::replace(const Char * s0, const Char * s1) {
    const UInt32 n0 = strlen(s0);
    const UInt32 n1 = strlen(s1);
    const UInt32 newSize = mSize - n0 + n1;
    if (newSize > mSize)
        mData = mData->edit(newSize + 1);
    else
        mData = mData->edit();

    Int index = indexOf(s0);
    Char * buf = mData->data();
    if (index >= 0) {
        if (n0 != n1) {
            memmove(buf + index + n1, buf + index + n0, mSize - index - n0 + 1);
        }
        memcpy(buf + index, s1, n1);
        mSize = mSize - n0 + n1;
    }
    return *this;
}

String& String::replaceAll(const Char * s0, const Char * s1) {
    // TODO: optimize these code
    for (;;) {
        Int index = indexOf(s0);
        if (index < 0) break;

        replace(s0, s1);
    }

    return *this;
}

Int String::indexOf(UInt32 start, const Char * s) const {
    const Char *sub = strstr(mData->data() + start, s);
    if (!sub) return -1;
    return sub - mData->data();
}

Int String::indexOf(UInt32 fromIndex, int c) const {
    Char * buf = mData->data();
    const Char *sub = strchr(buf + fromIndex, c);
    if (!sub) return -1;
    return sub - buf;
}

Int String::lastIndexOf(const Char *s) const {
    const UInt32 n = strlen(s);
    if (n > mSize)  return -1;
    
    const Char * buf = mData->data();
    for (UInt32 i = 0; i < mSize - n; ++i) {
        
        UInt32 j = 0;
        for (; j < n; j++) {
            if (buf[mSize - n - i + j] != s[j]) {
                break;
            }
        }
        
        if (j == n) return mSize - n - i;
    }
    return -1;
}

Int String::lastIndexOf(int c) const {
    Char * buf = mData->data();
    const Char *sub = strrchr(buf, c);
    if (!sub) return -1;
    return sub - buf;
}

int String::compare(const String& s) const {
    if (mData == s.mData) return 0;
    if (s.mData == Nil) return 1;
    if (mData == Nil) return -1;
    
    return strcmp(mData->data(), s.mData->data());
}

int String::compare(const Char *s) const {
    if (mData == Nil) return -1;
    return strcmp(mData->data(), s);
}

int String::compareIgnoreCase(const String& s) const {
    if (s.mData == Nil) return 1;
    if (mData == Nil) return -1;
    
    return strcasecmp(mData->data(), s.mData->data());
}

int String::compareIgnoreCase(const Char *s) const {
    if (mData == Nil) return -1;
    return strcasecmp(mData->data(), s);
}

String& String::lower() {
    mData = mData->edit();
    Char * buf = mData->data();
    for (UInt32 i = 0; i < mSize; ++i) {
        buf[i] = ::tolower(buf[i]);
    }
    return *this;
}

String& String::upper() {
    mData = mData->edit();
    Char * buf = mData->data();
    for (UInt32 i = 0; i < mSize; ++i) {
        buf[i] = ::toupper(buf[i]);
    }
    return *this;
}

Bool String::startsWith(const Char * s, UInt32 n) const {
    if (!n) n = strlen(s);
    if (n > mSize) return False;
    return !strncmp(mData->data(), s, n);
}

Bool String::startsWithIgnoreCase(const Char * s, UInt32 n) const {
    if (!n) n = strlen(s);
    if (n > mSize) return False;
    return !strncasecmp(mData->data(), s, n);
}

Bool String::endsWith(const Char * s, UInt32 n) const {
    if (!n) n = strlen(s);
    if (n > mSize) return False;
    return !strcmp(mData->data() + mSize - n, s);
}

Bool String::endsWithIgnoreCase(const Char * s, UInt32 n) const {
    if (!n) n = strlen(s);
    if (n > mSize) return False;
    return !strcasecmp(mData->data() + mSize - n, s);
}

String String::substring(UInt32 pos, UInt32 n) const {
    CHECK_TRUE(pos < mSize);
    Char * buf = mData->data();
    return String(buf + pos, n);
}

Int32 String::toInt32() const {
    return strtol(mData->data(), Nil, 10);
}

Int64 String::toInt64() const {
    return strtoll(mData->data(), Nil, 10);
}

Float32 String::toFloat() const {
    return strtof(mData->data(), Nil);
}

Float64 String::toDouble() const {
    return strtod(mData->data(), Nil);
}

void String::swap(String& s) {
    if (mData == s.mData) return;
    UInt32 size         = mSize;
    SharedBuffer* data  = mData;

    mSize       = s.mSize;
    mData       = s.mData;
    s.mSize     = size;
    s.mData     = data;
}

__END_NAMESPACE_ABE
