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

#ifndef ABE_HEADERS_MESSAGE_H
#define ABE_HEADERS_MESSAGE_H 

#include <ABE/core/Types.h>
#include <ABE/core/String.h>
#include <ABE/stl/HashTable.h>

__BEGIN_NAMESPACE_ABE

// we prefer FOURCC as name
class ABE_EXPORT Message : public SharedObject {
    public:
        Message(uint32_t what = 0);
        virtual ~Message();
    
        sp<Message> dup() const;
    
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
        ABE_INLINE uint32_t    what        () const { return mWhat; }
        ABE_INLINE size_t      countEntries() const { return mEntries.size(); }

        void            clear       ();
        bool            contains    (uint32_t name) const;
        bool            remove      (uint32_t name);
        String          string      () const;
        bool            contains    (uint32_t name, Type) const;
        uint32_t        getEntryNameAt(size_t index, Type *type) const;

    public:
        // core types
        void            setInt32    (uint32_t name, int32_t value);                    // kTypeInt32
        void            setInt64    (uint32_t name, int64_t value);                    // kTypeInt64
        void            setFloat    (uint32_t name, float value);                      // kTypeFloat
        void            setDouble   (uint32_t name, double value);                     // kTypeDouble
        void            setPointer  (uint32_t name, void *value);                      // kTypePointer
        void            setString   (uint32_t name, const char *s, size_t len = 0);    // kTypeString
        void            setObject   (uint32_t name, SharedObject * object);            // kTypeObject

        template <class T> ABE_INLINE void setObject(uint32_t name, const sp<T>& o)
        { setObject(name, static_cast<SharedObject *>(o.get())); }

        int32_t         findInt32   (uint32_t name, int32_t def = 0) const;            // kTypeInt32
        int64_t         findInt64   (uint32_t name, int64_t def = 0) const;            // kTypeInt64
        float           findFloat   (uint32_t name, float def = 0) const;              // kTypeFloat
        double          findDouble  (uint32_t name, double def = 0) const;             // kTypeDouble
        void *          findPointer (uint32_t name, void * def = NULL) const;          // kTypePointer
        const char *    findString  (uint32_t name, const char * def = NULL) const;    // kTypeString
        SharedObject *  findObject  (uint32_t name, SharedObject * def = NULL) const;  // kTypeObject

        // alias
        ABE_INLINE void setString   (uint32_t name, const String &s)
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
        HashTable<uint32_t, Entry>  mEntries;
    
        DISALLOW_EVILS(Message);
};

__END_NAMESPACE_ABE
#endif  // ABE_HEADERS_MESSAGE_H

