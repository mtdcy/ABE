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


// File:    String.h
// Author:  mtdcy.chen
// Changes:
//          1. 20160701     initial version
//


#ifndef _TOOLKIT_HEADERS_STRING_H
#define _TOOLKIT_HEADERS_STRING_H

#include <ABE/basic/Types.h>
#include <ABE/basic/SharedBuffer.h>

#ifdef __cplusplus
__BEGIN_NAMESPACE_ABE

#ifndef __APPLE__
typedef uint16_t char16_t;
#endif

class __ABE_EXPORT String : public SharedObject {
    public:
        /**
         * format a string
         */
        static String format(const char *format, ...);
        static String format(const char *format, va_list ap);

    public:
        /**
         * create an empty string
         */
        String();
        /**
         * copy a string
         * @param s     reference of another string
         */
        String(const String& s);
        /**
         * duplicate a null terminated c-style string
         * @param s     pointer to a c-style string
         * @param n     length of that string excluding terminating null
         */
        String(const char *s, size_t n = 0);
        /**
         * duplicate a null terminated utf16 c-style string
         * @param s     pointer to a c-style string
         * @param n     size of that string excluding terminating null
         */
        String(const char16_t *s, size_t n = 0);

        /**
         * create string from basic types
         */
        explicit String(const char c);
        explicit String(const uint8_t v);
        explicit String(const int8_t v);
        explicit String(const uint16_t v);
        explicit String(const int16_t v);
        explicit String(const uint32_t v);
        explicit String(const int32_t v);
        explicit String(const uint64_t v);
        explicit String(const int64_t v);
        explicit String(const float v);
        explicit String(const double v);
#ifdef  __APPLE__
        explicit String(const size_t v);
#endif
        explicit String(const void *p);

        ~String();

    public:
        // edit
        String&     set(const String& s);
        String&     append(const String &s);
        String&     insert(size_t pos, const String &s);
        String&     erase(size_t pos, size_t n);
        String&     replace(const String& s0, const String& s1);
        String&     replaceAll(const String& s0, const String& s1);
        String&     trim();
        void        clear();
        String      substring(size_t pos, size_t n = 0) const;
        void        swap(String& s);
        String&     lower();
        String&     upper();

    public:
        __ABE_INLINE String& operator=(const String &s)  { set(s); return *this;                 }
        __ABE_INLINE String& operator+=(const String &s) { append(s); return *this;              }
        __ABE_INLINE String  operator+(const String &s)  { String a(*this); return a.append(s);  }

    public:
        /**
         * get char at position
         * @param index     position, range, [0, size()]
         * @return return a char reference.
         */
        const char& operator[](size_t index) const;
        char&       operator[](size_t index);

    public:
        /**
         * return a pointer to null-terminated c-stype string
         * @note always non-null, even size() == 0
         * @note no non-const version of c_str()
         */
        __ABE_INLINE const char * c_str() const  { return mData->data(); }
        __ABE_INLINE size_t  size() const        { return mSize;         }
        __ABE_INLINE bool    empty() const       { return mSize == 0;    }

    public:
        ssize_t     indexOf(size_t start, const String& s) const;
        ssize_t     indexOf(size_t start, int c) const;
        ssize_t     lastIndexOf(const String& s) const;
        ssize_t     lastIndexOf(int c) const;

        __ABE_INLINE ssize_t indexOf(const String& s) const  { return indexOf(0, s); }
        __ABE_INLINE ssize_t indexOf(int c) const            { return indexOf(0, c); }

    public:
        size_t      hash() const;

    public:
        int         compare(const String &s) const ;
        int         compareIgnoreCase(const String &s) const;

    public:
        __ABE_INLINE bool equals(const String &s) const { return !compare(s); }
        __ABE_INLINE bool equalsIgnoreCase(const String &s) const { return !compareIgnoreCase(s); }

    public:
#define OPERATOR(op) __ABE_INLINE bool operator op(const String& rhs) const { return compare(rhs) op 0; }
        OPERATOR(==);
        OPERATOR(!=);
        OPERATOR(<);
        OPERATOR(<=);
        OPERATOR(>);
        OPERATOR(>=);
#undef OPERATOR

    public:
        bool        startsWith(const String &s) const;
        bool        startsWithIgnoreCase(const String &s) const;
        bool        endsWith(const String &s) const;
        bool        endsWithIgnoreCase(const String &s) const;

    public:
        int32_t     toInt32() const;
        int64_t     toInt64() const;
        float       toFloat() const;
        double      toDouble() const;

    public:
        String      dirname() const;
        String      basename() const;

    private:
        SharedBuffer*   mData;
        size_t          mSize;
};

///////////////////////////////////////////////////////////////////////////

__END_NAMESPACE_ABE
#endif

#endif // _TOOLKIT_HEADERS_STRING_H


