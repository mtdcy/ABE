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
#include "ABE/basic/Log.h"
#include "String.h"

#include "private/ConvertUTF.h"

#include <ctype.h> // isspace 
#include <strings.h> 
#include <string.h> 
#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>

static __ABE_INLINE size_t CStringPrintf(void *str, size_t size, const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    size_t len = vsnprintf((char*)str, size, format, ap);
    va_end(ap);
    return len;
}

// about cow 
// http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2008/n2534.html

// Note: 
// 1. no CHECK_XX can be used in String's constructor, will cause loop

__BEGIN_NAMESPACE_ABE

static __ABE_INLINE size_t strlen16(const char16_t *s) {
    const char16_t *ss = s;
    while (*ss) ss++;
    return ss - s;
}

String String::format(const char *format, ...) {
    va_list ap;
    char buf[1024];
    va_start(ap, format);

    size_t len = vsnprintf(buf, 1024, format, ap);

    va_end(ap);

    String result(buf, len);
    return result;
}

String String::format(const char *format, va_list ap) {
    char buf[1024];
    size_t len = vsnprintf(buf, 1024, format, ap);
    String result(buf, len);
    return result;
}

String::String() : mData(NULL), mSize(0)
{
    mData = SharedBuffer::Create(kAllocatorDefault, 1);
    char * buf = mData->data();
    buf[0] = '\0';
}

String::String(const char *s, size_t n)
    : mData(NULL), mSize(0)
{
    CHECK_NULL(s);
    if (!n) mSize = strlen(s);
    else    mSize = strnlen(s, n);
    mData = SharedBuffer::Create(kAllocatorDefault, mSize + 1);
    char * buf = mData->data();
    if (mSize) memcpy(buf, s, mSize);
    buf[mSize] = '\0';
}

String::String(const String &from)
    : mData(NULL), mSize(0)
{
    mData   = from.mData->RetainBuffer();
    mSize   = from.mSize;
}

String::String(const char16_t *s, size_t n) 
    : mData(NULL), mSize(0)
{
    CHECK_NULL(s);
    if (n == 0)  n = strlen16(s);

    // is the size right?
    mData = SharedBuffer::Create(kAllocatorDefault, n * 4 + 1);
    char * buf_start = mData->data();
    char * buf_end = buf_start + mData->size();
    char * buf = buf_start;

    ConversionResult result = ConvertUTF16toUTF8(
            (const UTF16**)&s, (const UTF16*)(s + n),
            (UTF8**)&buf,
            (UTF8*)buf_end,
            lenientConversion);
    if (result != conversionOK) {
        ERROR("ConvertUTF16toUTF8 failed, ret = %d.", result);
        return;
    }

    mSize = buf - buf_start;
    buf_start[mSize] = '\0';
}

#define STRING_FROM_NUMBER(TYPE, SIZE, PRI)                                 \
    String::String(const TYPE v) : mData(NULL), mSize(0) {                  \
        mData = SharedBuffer::Create(kAllocatorDefault, SIZE + 1);          \
        char * buf = mData->data();                                         \
        int result = CStringPrintf(buf, SIZE, "%" PRI, v);                  \
        CHECK_GT(result, 0); CHECK_LE(result, SIZE);                        \
        mSize               = result;                                       \
        buf[mSize]          = '\0';                                         \
    }

STRING_FROM_NUMBER(char,        2,      "c");
STRING_FROM_NUMBER(uint8_t,     4,      PRIu8);
STRING_FROM_NUMBER(int8_t,      4,      PRId8);
STRING_FROM_NUMBER(uint16_t,    6,      PRIu16);
STRING_FROM_NUMBER(int16_t,     6,      PRId16);
STRING_FROM_NUMBER(uint32_t,    16,     PRIu32);
STRING_FROM_NUMBER(int32_t,     16,     PRId32);
STRING_FROM_NUMBER(uint64_t,    32,     PRIu64);
STRING_FROM_NUMBER(int64_t,     32,     PRId64);
STRING_FROM_NUMBER(float,       32,     "f");
STRING_FROM_NUMBER(double,      64,     "f");
#ifdef __APPLE__
STRING_FROM_NUMBER(size_t,      16,     "zu");
#endif
STRING_FROM_NUMBER(void *,      32,     "xp");

String::~String() {
    mData->ReleaseBuffer();
    mData = NULL;
}

String String::basename() const {
    char * buf = mData->data();
    const char *last = strrchr(buf, '/');
    if (last) {
        const char *dot = strrchr(last, '.');
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

const char& String::operator[](size_t index) const {
    CHECK_LE(index, size());
    char * buf = mData->data();
    return buf[index];
}

char& String::operator[](size_t index) {
    CHECK_LE(index, size());
    mData = mData->edit();
    char * buf = mData->data();
    return buf[index];
}

String& String::set(const String& s) {
    mData->ReleaseBuffer();
    mData   = s.mData->RetainBuffer();
    mSize   = s.mSize;
    return *this;
}

String& String::append(const String& s) {
    if (!s.mSize) return *this; // append empty string ?

    if (!mSize) return set(s);

    size_t m = mSize + s.mSize + 1;
    mData = mData->edit(m);

    // plus 1 for '\0'
    char * buf = mData->data();
    strncpy(buf + mSize,
            s.mData->data(),
            s.mSize + 1);
    mSize += s.mSize;
    return *this;
}

String& String::insert(size_t pos, const String& s) {
    CHECK_LE(pos, mSize);
    if (!s.mSize) return *this; // insert an empty string ???

    if (pos == mSize) return append(s);
    if (!mSize) return set(s);

    size_t m = mSize + s.mSize + 1;
    mData = mData->edit(m);

    // +1 to move the terminating null byte
    char * buf = mData->data();
    memmove(buf + pos + s.mSize,
            buf + pos,
            mSize - pos + 1);
    strncpy(buf + pos,
            s.mData->data(),
            s.mSize);
    mSize   += s.mSize;
    return *this;
}

void String::clear() {
    mData = mData->edit();
    char * buf = mData->data();
    mSize = 0;
    buf[mSize] = '\0';
    // no need to clear memory
}

size_t String::hash() const {
    size_t x = 0;
    const char *s = mData->data();
    for (size_t i = 0; i < mSize; ++i) {
        x = (x * 31) + s[i];
    }
    return x;
}

String& String::trim() {
    if (mSize == 0) return *this;

    mData = mData->edit();

    char *s = mData->data();
    size_t i = 0;
    while (i < mSize && isspace(s[i])) {
        ++i;
    }

    size_t j = mSize;
    while (j > i && isspace(s[j - 1])) {
        --j;
    }

    memmove(s,
            &s[i],
            j - i);
    mSize = j - i;
    s[mSize] = '\0';
    return *this;
}

String& String::erase(size_t pos, size_t n) {
    CHECK_TRUE(pos < mSize);
    CHECK_TRUE(pos + n <= mSize);

    mData = mData->edit();
    char * buf = mData->data();

    if (pos + n < mSize) {
        memmove(buf + pos,
                buf + pos + n,
                mSize - (pos + n));
    }
    mSize -= n;
    buf[mSize] = '\0';
    return *this;
}

String& String::replace(const String& s0, const String& s1) {
    const size_t newSize = mSize - s0.mSize + s1.mSize;
    if (newSize > mSize)
        mData = mData->edit(newSize + 1);
    else
        mData = mData->edit();

    ssize_t index = indexOf(s0);
    char * buf = mData->data();
    if (index >= 0) {
        if (s0.mSize != s1.mSize) {
            memmove(buf + index + s1.mSize,
                    buf + index + s0.mSize,
                    mSize - index - s0.mSize + 1);
        }
        memcpy(buf + index,
                s1.mData->data(),
                s1.mSize);
        mSize = mSize - s0.mSize + s1.mSize;
    }
    return *this;
}

String& String::replaceAll(const String& s0, const String& s1) {
    // TODO: optimize these code
    for (;;) {
        ssize_t index = indexOf(s0);
        if (index < 0) break;

        replace(s0, s1);
    }

    return *this;
}

ssize_t String::indexOf(size_t fromIndex, const String& s) const {
    char * buf = mData->data();
    const char *sub = strstr(buf + fromIndex,
            s.mData->data());
    if (!sub) return -1;
    return sub - buf;
}

ssize_t String::indexOf(size_t fromIndex, int c) const {
    char * buf = mData->data();
    const char *sub = strchr(buf + fromIndex, c);
    if (!sub) return -1;
    return sub - buf;
}

ssize_t String::lastIndexOf(const String& s) const {
    char * buf = mData->data();
    const char *last = NULL;
    const char *cur = strstr(buf,
            s.mData->data());
    while (cur) {
        //INFO("cur: %s", cur);
        last = cur;
        cur = strstr(last + s.mSize, s.mData->data());
    }
    if (!last) return -1;
    return last - buf;
}

ssize_t String::lastIndexOf(int c) const {
    char * buf = mData->data();
    const char *sub = strrchr(buf, c);
    if (!sub) return -1;
    return sub - buf;
}

int String::compare(const String& s) const {
    char * buf = mData->data();
    return strcmp(buf, s.mData->data());
}

int String::compareIgnoreCase(const String& s) const {
    char * buf = mData->data();
    return strcasecmp(buf, s.mData->data());
}

String& String::lower() {
    mData = mData->edit();
    char * buf = mData->data();
    for (size_t i = 0; i < mSize; ++i) {
        buf[i] = ::tolower(buf[i]);
    }
    return *this;
}

String& String::upper() {
    mData = mData->edit();
    char * buf = mData->data();
    for (size_t i = 0; i < mSize; ++i) {
        buf[i] = ::toupper(buf[i]);
    }
    return *this;
}

bool String::startsWith(const String& s) const {
    if (s.mSize > mSize) return false;
    char * buf = mData->data();
    return !strncmp(buf,
            s.mData->data(),
            s.mSize);
}

bool String::startsWithIgnoreCase(const String& s) const {
    if (s.mSize > mSize) return false;
    char * buf = mData->data();
    return !strncasecmp(buf,
            s.mData->data(),
            s.mSize);
}

bool String::endsWith(const String& s) const {
    if (s.mSize > mSize) return false;
    char * buf = mData->data();
    return !strcmp(buf + mSize - s.mSize,
            s.mData->data());
}

bool String::endsWithIgnoreCase(const String& s) const {
    if (s.mSize > mSize) return false;
    char * buf = mData->data();
    return !strcasecmp(buf + mSize - s.mSize,
            s.mData->data());
}

String String::substring(size_t pos, size_t n) const {
    CHECK_TRUE(pos < mSize);
    char * buf = mData->data();
    return String(buf + pos, n);
}

int32_t String::toInt32() const {
    return strtol(mData->data(), NULL, 10);
}

int64_t String::toInt64() const {
    return strtoll(mData->data(), NULL, 10);
}

float String::toFloat() const {
    return strtof(mData->data(), NULL);
}

double String::toDouble() const {
    return strtod(mData->data(), NULL);
}

void String::swap(String& s) {
    if (mData == s.mData) return;
    size_t size         = mSize;
    SharedBuffer* data  = mData;

    mSize       = s.mSize;
    mData       = s.mData;
    s.mSize     = size;
    s.mData     = data;
}

__END_NAMESPACE_ABE
