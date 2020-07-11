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


// File:    test.cpp
// Author:  mtdcy.chen
// Changes:
//          1. 20160701     initial version

#define REMOVE_ATOMICS
#define LOG_TAG "Toolkit"
#include <ABE/ABE.h>

#include <gtest/gtest.h>
#include <inttypes.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>

#include <unistd.h>

USING_NAMESPACE_ABE

static const Char * gCurrentDir = Nil;

struct MyTest : public ::testing::Test {
    MyTest () {
    }

    virtual ~MyTest() {
    }

    virtual void SetUp() {
    }

    virtual void TearDown() {
    }
};

struct Integer {
    int value;
    Integer() : value(0) { }
    Integer(int i) : value(i) { }
    Bool operator<(const Integer& rhs) const    { return value < rhs.value;     }
    Bool operator==(int i) const                { return value == i;            }
    Bool operator==(const Integer& rhs) const   { return value == rhs.value;    }
    Integer& operator++()                       { ++value; return *this;        }   // pre-increment
    Integer  operator++(int)                    { return value++;               }   // dispatch-increment
};

template <class TYPE> static Bool LessCompare(const TYPE* lhs, const TYPE* rhs) {
    return *lhs < *rhs;
}

struct MySharedObject : public SharedObject {
    MySharedObject() { }
};

void testAtomic() {
    Atomic<int> a;
    ASSERT_EQ(a.load(), 0);

    a.store(1);
    ASSERT_EQ(a.load(), 1);
    ASSERT_EQ(a++, 1);  // a == 2
    ASSERT_EQ(++a, 3);  // a == 3
    ASSERT_EQ(a--, 3);  // a == 2
    ASSERT_EQ(--a, 1);  // a == 1
}

void testSharedObject() {
    // api test for template
    {
        // default constructor
        sp<SharedObject> sp0;
        sp0 = Nil;
    }
    
    { // sp<T>
        // constructor
        sp<SharedObject> sp0 = new MySharedObject;  // sp<T>(SharedObject *)
        sp<SharedObject> sp1 = sp0;                 // sp<T>(const sp<T>&)
        sp<MySharedObject> sp2 = sp1;               // sp<T>(const sp<U>&)
        
        // copy assignment
        sp0 = new MySharedObject;                   // implicit convert MySharedObject * -> sp<SharedObject>)
        sp1 = sp0;                                  // sp<T>& operator=(const sp<T>&)
        sp2 = sp1;                                  // sp<T>& operator(const sp<U>&)
    }
    
    {
        // default constructor
        wp<SharedObject> wp0;
        wp0 = Nil;
    }
    { // wp<T>
        // constructor
        MySharedObject * object = new MySharedObject;
        sp<SharedObject> sp0 = object;
        wp<SharedObject> wp0 = object;          // wp<T>(SharedObject *)
        wp<SharedObject> wp1 = wp0;             // wp<T>(const wp<T>&)
        wp<SharedObject> wp2 = sp0;             // wp<T>(const sp<T>&)
        wp<MySharedObject> wp3 = wp2;           // wp<T>(const wp<U>&)
        wp<MySharedObject> wp4 = sp0;           // wp<T>(const sp<U>&)
        
        // copy assignment
        wp0 = object;                           // implicit convert MySharedObject * -> wp<SharedObject>
        wp1 = wp0;                              // wp<T>& operator=(const wp<T>&)
        wp2 = sp0;                              // wp<T>& operator=(const sp<T>&)
        wp3 = wp2;                              // wp<T>& operator=(const wp<U>&)
        wp4 = sp0;                              // wp<T>& operator=(const sp<U>&)
    }
    
    // logic test
    SharedObject * object = new MySharedObject;
    ASSERT_EQ(object->GetRetainCount(), 0);

    sp<SharedObject> sp1 = object;
    ASSERT_FALSE(sp1.isNil());
    ASSERT_EQ(object->GetRetainCount(), 1);
    ASSERT_EQ(sp1.refsCount(), 1);
    ASSERT_TRUE(sp1.get() != Nil);

    sp<SharedObject> sp2 = sp1;
    ASSERT_EQ(object->GetRetainCount(), 2);
    ASSERT_EQ(sp2.refsCount(), 2);
    
    wp<SharedObject> wp1 = sp1;
    ASSERT_EQ(wp1.refsCount(), 3);
    ASSERT_EQ(sp1.refsCount(), 2);
    ASSERT_TRUE(wp1.retain() != Nil);
    wp<SharedObject> wp2 = wp1;
    ASSERT_EQ(wp2.refsCount(), 4);
    ASSERT_EQ(sp1.refsCount(), 2);

    sp2.clear();
    ASSERT_TRUE(sp2.isNil());
    ASSERT_EQ(object->GetRetainCount(), 1);
    ASSERT_EQ(sp1.refsCount(), 1);
    ASSERT_TRUE(sp2.get() == Nil);
    
    wp1.clear();
    ASSERT_TRUE(wp1.isNil());
    ASSERT_EQ(wp2.refsCount(), 2);
}

void testAllocator() {
    sp<Allocator> allocator = kAllocatorDefault;
    void * p = allocator->allocate(1024);
    ASSERT_TRUE(p != Nil);

    p = allocator->reallocate(p, 2048);
    ASSERT_TRUE(p != Nil);

    allocator->deallocate(p);
}

void testString() {
    const Char * STRING = "abcdefghijklmn";
    const String s0;
    const String s1 = STRING;
    const String s2 = "ABCDEFGHIJKLMN";
    const String s3 = "abcdefgabcdefg";
    const String s4 = STRING;
    const UInt32 n = strlen(STRING);

    // check on empty string
    ASSERT_EQ(s0.size(), 0);
    ASSERT_TRUE(s0.empty());
    //ASSERT_EQ(s0[0], '\0');
    //ASSERT_TRUE(s0.c_str() != Nil);
    //ASSERT_STREQ(s0.c_str(), "");

    // check on non-empty string
    ASSERT_EQ(s1.size(), 14);
    ASSERT_FALSE(s1.empty());
    ASSERT_TRUE(s1.c_str() != Nil);
    ASSERT_STREQ(s1.c_str(), STRING);
    for (UInt32 i = 0; i < n; ++i) {
        ASSERT_EQ(s1[i], STRING[i]);
    }
    ASSERT_EQ(s1[n], '\0');

    // compare
    ASSERT_TRUE(s1 != s0);
    ASSERT_TRUE(s1 == s4);
    ASSERT_FALSE(s1.equals(s0));
    ASSERT_TRUE(s1.equals(s4));
    ASSERT_GT(s1.compare(s0), 0);
    ASSERT_EQ(s1.compare(s4), 0);
    ASSERT_LT(s0.compare(s1), 0);

    // compare without case
    ASSERT_TRUE(s1.equals(s2, True));
    ASSERT_EQ(s1.compare(s2, True), 0);

    // indexOf
    ASSERT_EQ(s3.indexOf("a"), 0);
    ASSERT_EQ(s3.indexOf("b"), 1);
    ASSERT_EQ(s3.indexOf(7, "c"), 9);
    ASSERT_EQ(s3.lastIndexOf("a"), 7);
    ASSERT_EQ(s3.lastIndexOf("b"), 8);

    // start & end with
    ASSERT_TRUE(s1.startsWith("abc"));
    ASSERT_FALSE(s1.startsWith("aaa"));
    ASSERT_TRUE(s1.endsWith("lmn"));
    ASSERT_FALSE(s1.endsWith("aaa"));
    ASSERT_TRUE(s2.startsWith("abc", True));
    ASSERT_FALSE(s2.startsWith("aaa", True));
    ASSERT_TRUE(s2.endsWith("lmn", True));
    ASSERT_FALSE(s2.endsWith("aaa", True));

    // upper & lower
    {
        String tmp = s1;
        ASSERT_TRUE(tmp.upper() == s2);
        ASSERT_TRUE(tmp.lower() == s1);
    }

    // string copy & edit
    {
        String tmp;
        tmp.set(STRING);
        CHECK_EQ(tmp, s1);
        ASSERT_TRUE(tmp == s1);
        tmp.clear();
        ASSERT_TRUE(tmp == s0);
        tmp.append("abcdefg");
        tmp.append("hijklmn");
        ASSERT_TRUE(tmp == s1);
        tmp.erase(0, 3);
        ASSERT_STREQ(tmp.c_str(), "defghijklmn");
        tmp.erase(3, 8);
        ASSERT_STREQ(tmp.c_str(), "def");
        tmp = s1;
        tmp.replace("abc", "cba");
        ASSERT_STREQ(tmp.c_str(), "cbadefghijklmn");
        tmp = s3;
        tmp.replace("abc", "cba", True);
        ASSERT_STREQ(tmp.c_str(), "cbadefgcbadefg");
        tmp = " abcdefghijklmn ";
        tmp.trim();
        ASSERT_TRUE(tmp == s1);
    }
}

void testBits() {
    Bits<UInt8> bits;
    ASSERT_EQ(bits.value(), 0);
    
    bits.set(1);
    ASSERT_EQ(bits.value(), 0x02);
    ASSERT_TRUE(bits.test(1));
    ASSERT_FALSE(bits.empty());
    
    bits.clear(2);
    ASSERT_EQ(bits.value(), 0x02);
    bits.clear(1);
    ASSERT_EQ(bits.value(), 0x00);
    
    bits = 0xF0;
    ASSERT_EQ(bits.value(), 0xF0);
    bits.flip(7);
    ASSERT_EQ(bits.value(), 0x70);
    bits.flip();
    ASSERT_EQ(bits.value(), 0x8F);
    
    bits.clear();
    ASSERT_EQ(bits.value(), 0x00);
    ASSERT_TRUE(bits.empty());
}

// at least 66 + 36 = 102 bytes
void testABuffer(sp<ABuffer> base) {
    for (UInt32 i = 0; i < 32; ++i) {
        base->write(i, i + 1);
    }
    base->flushBytes();
    // 66 bytes
    ASSERT_EQ(base->size(), 66);
    
    for (UInt32 i = 0; i < 32; ++i) {
        ASSERT_EQ(base->read(i + 1), i);
    }
    ASSERT_EQ(base->size(), 0);
    
    base->w8(0xF0);
    ASSERT_EQ(base->r8(), 0xF0);
    base->wl16(0xF0);
    ASSERT_EQ(base->rl16(), 0xF0);
    base->wl24(0xF0);
    ASSERT_EQ(base->rl24(), 0xF0);
    base->wl32(0xF0);
    ASSERT_EQ(base->rl32(), 0xF0);
    base->wl64(0xF0);
    ASSERT_EQ(base->rl64(), 0xF0);
    // + 36 bytes
}

void testBuffer() {
    testABuffer(new Buffer(102));
    testABuffer(new Buffer(102, Buffer::Ring));
    
    // test linear buffer
    sp<Buffer> buffer = new Buffer(16);
    ASSERT_EQ(buffer->type(), Buffer::Linear);
    ASSERT_EQ(buffer->capacity(), 16);
    ASSERT_EQ(buffer->empty(), 16);
    ASSERT_EQ(buffer->size(), 0);
    ASSERT_EQ(buffer->offset(), 0);
    
    buffer->writeBytes("abcdefgh");  // write 8 bytes
    ASSERT_TRUE(memcmp(buffer->data(), "abcdefgh", 8) == 0);
    ASSERT_EQ(buffer->capacity(), 16);
    ASSERT_EQ(buffer->empty(), 8);
    ASSERT_EQ(buffer->size(), 8);
    ASSERT_EQ(buffer->offset(), 0);
    
    sp<Buffer> data = buffer->readBytes(8);
    ASSERT_TRUE(memcmp(data->data(), "abcdefgh", 8) == 0);
    ASSERT_EQ(buffer->capacity(), 16);
    ASSERT_EQ(buffer->empty(), 8);
    ASSERT_EQ(buffer->size(), 0);
    ASSERT_EQ(buffer->offset(), 8);
    data.clear();
    
    buffer->writeBytes("abcdefgh");
    ASSERT_TRUE(memcmp(buffer->data(), "abcdefgh", 8) == 0);
    ASSERT_EQ(buffer->capacity(), 16);
    ASSERT_EQ(buffer->empty(), 0);
    ASSERT_EQ(buffer->size(), 8);
    ASSERT_EQ(buffer->offset(), 8);
    
    buffer->skipBytes(8);
    ASSERT_EQ(buffer->capacity(), 16);
    ASSERT_EQ(buffer->empty(), 0);
    ASSERT_EQ(buffer->size(), 0);
    ASSERT_EQ(buffer->offset(), 16);
    
    // cow test
    buffer->resetBytes();   // put read pointer back to begin
    buffer->skipBytes(8);
    data = buffer->readBytes(8);    // read 8 bytes @ offset 8
    buffer->clearBytes();
    buffer->writeBytes("hgfedcbahgfedcba");
    ASSERT_TRUE(memcmp(buffer->data(), "hgfedcbahgfedcba", 16) == 0);
    ASSERT_EQ(buffer->type(), Buffer::Linear);
    ASSERT_EQ(buffer->capacity(), 16);
    ASSERT_EQ(buffer->empty(), 0);
    ASSERT_EQ(buffer->size(), 16);
    ASSERT_EQ(buffer->offset(), 0);
    // resize test
    buffer->resize(buffer->capacity() * 2);
    ASSERT_TRUE(memcmp(buffer->data(), "hgfedcbahgfedcba", 16) == 0);
    ASSERT_EQ(buffer->type(), Buffer::Linear);
    ASSERT_EQ(buffer->capacity(), 32);
    ASSERT_EQ(buffer->empty(), 16);
    ASSERT_EQ(buffer->size(), 16);
    ASSERT_EQ(buffer->offset(), 0);
    
    // cow test
    ASSERT_TRUE(memcmp(data->data(), "abcdefgh", 8) == 0);
    ASSERT_EQ(data->type(), Buffer::Linear);
    ASSERT_EQ(data->capacity(), 8);
    ASSERT_EQ(data->empty(), 0);
    ASSERT_EQ(data->size(), 8);
    ASSERT_EQ(data->offset(), 0);
    
    
    // Ring Buffer
    buffer = new Buffer(16, Buffer::Ring);
    ASSERT_EQ(buffer->type(), Buffer::Ring);
    ASSERT_EQ(buffer->capacity(), 16);
    ASSERT_EQ(buffer->empty(), 16);
    ASSERT_EQ(buffer->size(), 0);
    ASSERT_EQ(buffer->offset(), 0);
    
    buffer->writeBytes("abcdefgh");  // write 8 bytes
    ASSERT_TRUE(memcmp(buffer->data(), "abcdefgh", 8) == 0);
    ASSERT_EQ(buffer->capacity(), 16);
    ASSERT_EQ(buffer->empty(), 8);
    ASSERT_EQ(buffer->size(), 8);
    ASSERT_EQ(buffer->offset(), 0);
    
    data = buffer->readBytes(8);
    ASSERT_TRUE(memcmp(data->data(), "abcdefgh", 8) == 0);
    ASSERT_EQ(buffer->capacity(), 16);
    ASSERT_EQ(buffer->empty(), 16);
    ASSERT_EQ(buffer->size(), 0);
    ASSERT_EQ(buffer->offset(), 8);
    data.clear();
    
    buffer->writeBytes("abcdefgh");
    ASSERT_TRUE(memcmp(buffer->data(), "abcdefgh", 8) == 0);
    ASSERT_EQ(buffer->capacity(), 16);
    ASSERT_EQ(buffer->empty(), 8);
    ASSERT_EQ(buffer->size(), 8);
    ASSERT_EQ(buffer->offset(), 8);
    
    buffer->skipBytes(8);
    ASSERT_EQ(buffer->capacity(), 16);
    ASSERT_EQ(buffer->empty(), 16);
    ASSERT_EQ(buffer->size(), 0);
    ASSERT_EQ(buffer->offset(), 16);
    
    // cow test on ring buffer
    buffer->resetBytes();
    buffer->skipBytes(8);
    data = buffer->readBytes(8);
    buffer->clearBytes();
    buffer->writeBytes("hgfedcbahgfedcba");
    ASSERT_TRUE(memcmp(buffer->data(), "hgfedcbahgfedcba", 16) == 0);
    ASSERT_EQ(buffer->type(), Buffer::Ring);
    ASSERT_EQ(buffer->capacity(), 16);
    ASSERT_EQ(buffer->empty(), 0);
    ASSERT_EQ(buffer->size(), 16);
    ASSERT_EQ(buffer->offset(), 0);
    
    // resize test
    buffer->resize(buffer->capacity() * 2);
    ASSERT_TRUE(memcmp(buffer->data(), "hgfedcbahgfedcba", 16) == 0);
    ASSERT_EQ(buffer->type(), Buffer::Ring);
    ASSERT_EQ(buffer->capacity(), 32);
    ASSERT_EQ(buffer->empty(), 16);
    ASSERT_EQ(buffer->size(), 16);
    ASSERT_EQ(buffer->offset(), 0);
    
    // cow test
    ASSERT_TRUE(memcmp(data->data(), "abcdefgh", 8) == 0);
    ASSERT_EQ(data->type(), Buffer::Linear);
    ASSERT_EQ(data->capacity(), 8);
    ASSERT_EQ(data->empty(), 0);
    ASSERT_EQ(data->size(), 8);
    ASSERT_EQ(data->offset(), 0);
}

struct EmptySharedObject : public SharedObject {
    EmptySharedObject() : SharedObject() { }
    virtual ~EmptySharedObject() { }
};

void testMessage() {
    Message message;
    SharedObject * object = new EmptySharedObject;
    
#define TEST_MESSAGE(NAME, VALUE) {                     \
    Message msg;                                        \
    ASSERT_FALSE(msg.contains('test'));                 \
    msg.set##NAME('test', VALUE);                       \
    ASSERT_EQ(msg.find##NAME('test'), VALUE);           \
    ASSERT_TRUE(msg.contains('test'));                  \
}

    TEST_MESSAGE(Int32, 32)
    TEST_MESSAGE(Int64, 64)
    TEST_MESSAGE(Float, 1.0)
    TEST_MESSAGE(Double, 2.0)
    TEST_MESSAGE(Pointer, &message);
    TEST_MESSAGE(Object, object)
    
#undef TEST_MESSAGE
    
    const Char * string = "abcdefg";
    message.setString('str ', string);
    ASSERT_TRUE(message.contains('str '));
    ASSERT_STREQ(message.findString('str '), string);
    
    message.clear();
}

struct ThreadJob : public Job {
    const String name;
    Atomic<UInt32> count;
    ThreadJob(const String& _name) : name(_name), count(0) { }
    virtual void onJob() {
        INFO("ThreadJob %s", name.c_str());
        ++count;
    }
};

struct MainLooperAssist : public Job {
    virtual void onJob() {
        INFO("dispatch prepare");
        SleepTimeMs(1000); // 1s
        sp<Looper> main = Looper::Main();
        
        INFO("dispatch assist 0");
        main->dispatch(new ThreadJob("assist 0"));
        
        INFO("dispatch assist 1");
        main->dispatch(new ThreadJob("assist 1"), 500000LL);   // 500ms
        SleepTimeMs(10);
        INFO("dispatch assist 2");
        main->dispatch(new ThreadJob("assist 2"));
        
        main->terminate();
    }
};

void testLooper() {
    sp<Looper> looper1 = new Looper("Looper 1");
    looper1->dispatch(new ThreadJob("Looper run 1"));
    looper1.clear();

    sp<Looper> current = Looper::Current();
    ASSERT_TRUE(current != Nil);
    current.clear();

    sp<Looper> main = Looper::Main();
    sp<Looper> assist = new Looper("assist");
    assist->dispatch(new MainLooperAssist);
    ASSERT_TRUE(main != Nil);
    
    main->loop();
    main.clear();
    assist.clear();
    
    sp<Looper> lp = new Looper("looper0");
    sp<ThreadJob> job0 = new ThreadJob("job0");
    sp<ThreadJob> job1 = new ThreadJob("job1");
    for (UInt32 i = 0; i < 100; i++) {
        switch (i % 10) {
            case 0:
                lp->dispatch(job1);
                break;
            case 1:
                lp->dispatch(job1, 5000LL);     // 5ms
                break;
            case 2:
                lp->dispatch(job0, 10000LL);
                break;
            case 9:
                lp->remove(job1);
                break;
            case 3:
                ASSERT_TRUE(lp->exists(job0));
                ASSERT_TRUE(lp->exists(job1));
            default:
                lp->dispatch(job0);
                break;
        }
    }
    SleepTimeMs(200);
    lp.clear();
    SleepTimeMs(200);
    ASSERT_EQ(job0->count.load(), 10 * 7);
}

struct QueueJob : public Job {
    UInt32 count;
    QueueJob() : count(0) { }
    virtual void onJob() {
        INFO("on dispatch queue job %zu", count++);
    }
};

void testDispatchQueue() {
    sp<Looper> looper = new Looper("DispatchQueue");
    sp<DispatchQueue> disp0 = new DispatchQueue(looper);
    sp<DispatchQueue> disp1 = new DispatchQueue(looper);
    
    sp<Job> job = new QueueJob;
    disp0->dispatch(job);
    disp1->dispatch(job);
    
    SleepTimeMs(200);   // 200ms
    
    disp0->dispatch(job, 1000000LL);
    ASSERT_TRUE(disp0->exists(job));
    ASSERT_FALSE(disp1->exists(job));
    ASSERT_FALSE(looper->exists(job));
    
    disp0->remove(job);
    disp1->dispatch(job, 1000000LL);
    ASSERT_TRUE(disp1->exists(job));
    ASSERT_FALSE(disp0->exists(job));
    ASSERT_FALSE(looper->exists(job));
    
    disp0->dispatch(job, 1000000LL);
    disp1->flush();
    ASSERT_TRUE(disp0->exists(job));
    ASSERT_FALSE(disp1->exists(job));
    
    sp<QueueJob> job0 = new QueueJob;
    for (UInt32 i = 0; i < 100; ++i) {
        disp0->dispatch(job0);
    }
    SleepTimeMs(200);   // 200ms
    disp0.clear();
    ASSERT_EQ(job0->count, 100);
    
    disp1.clear();
    
    // test sync
    sp<QueueJob> job1 = new QueueJob;
    sp<DispatchQueue> disp3 = new DispatchQueue(looper);
    disp3->sync(job1);
}

template <class TYPE> struct QueueConsumer : public Job {
    LockFree::Queue<TYPE> mQueue;
    const int kCount;
    QueueConsumer(const sp<Looper>& lp) : Job(lp), kCount(10000) { }
    
    virtual void onJob() {
        int next = 0;
        for (;;) {
            TYPE value;
            if (mQueue.pop(value)) {
                ASSERT_TRUE(value == next);
                ++next;
                if (next == kCount) break;
                //if ((next % 10000) == 0) {
                //    INFO("queue size %zu", mQueue.size());
                //}
            } else {
                // loop without delay
            }
        }
        ASSERT_TRUE(mQueue.empty());
        INFO("End queue consumer");
    }
};

template <class TYPE> void testQueue() {
    LockFree::Queue<TYPE>  queue;
    ASSERT_EQ(queue.size(), 0);
    ASSERT_TRUE(queue.empty());

    queue.push(1);
    ASSERT_EQ(queue.size(), 1);
    queue.push(2);
    ASSERT_EQ(queue.size(), 2);

    TYPE value;
    queue.pop(value);
    ASSERT_EQ(value, 1);
    ASSERT_EQ(queue.size(), 1);
    queue.pop(value);
    ASSERT_EQ(value, 2);
    ASSERT_EQ(queue.size(), 0);

    // single producer & single consumer test
    sp<QueueConsumer<TYPE> > consumer = new QueueConsumer<TYPE>(new Looper("QueueConsumer"));
    consumer->dispatch();
    
    // producer
    for (TYPE i = 0; i < consumer->kCount; ++i) {
        consumer->mQueue.push(i);
    }
    
    SleepTimeMs(200);
    ASSERT_TRUE(consumer->mQueue.empty());
    consumer.clear();
    SleepTimeMs(200);
}

void testQueue1() { testQueue<int>();       }
void testQueue2() { testQueue<Integer>();   }

template <class TYPE> void testList() {
    List<TYPE> list;
    
    list.push(1);
    ASSERT_TRUE(list.front() == 1);
    ASSERT_TRUE(list.back() == 1);
    list.push(2);
    ASSERT_TRUE(list.front() == 1);
    ASSERT_TRUE(list.back() == 2);
    list.push(3);
    ASSERT_TRUE(list.front() == 1);
    ASSERT_TRUE(list.back() == 3);
    list.pop();
    ASSERT_TRUE(list.front() == 2);
    ASSERT_TRUE(list.back() == 3);
    list.pop();
    ASSERT_TRUE(list.front() == 3);
    ASSERT_TRUE(list.back() == 3);
    list.pop();
    ASSERT_TRUE(list.empty());

    // sort with predefined compare
    list.clear();
    for (int i = 0; i < 10 - i - 1; ++i) {
        list.push(i);
        if (i < 10 - i - 1) list.push(10 - i - 1);
    }
    ASSERT_EQ(list.size(), 10);
    list.sort();
    list.sort(LessCompare<TYPE>);   // just syntax check
    for (int i = 0; i < 10; ++i) {
        ASSERT_TRUE(list.front() == i);
        list.pop();
    }
}

void testList1() { testList<int>();     }
void testList2() { testList<Integer>(); }

template <class TYPE> void testVector() {
    Vector<TYPE> vec(4);
    ASSERT_EQ(vec.size(), 0);
    ASSERT_TRUE(vec.empty());
    for (int i = 0; i < 10; ++i) {
        if (i % 2)  vec.push(i);
        else        vec.push() = i;
        ASSERT_TRUE(vec.back() == i);
    }

    for (int i = 0; i < 10; ++i) {
        ASSERT_TRUE(vec[i] == i);
    }

    // vector copy
    Vector<TYPE> copy = vec;

    { // const vector copy
        const Vector<TYPE> cvec = vec;
        for (int i = 0; i < 10; ++i) {  // operator[] const
            ASSERT_TRUE(cvec[i] == i);
        }
    }

    // edit copy
    for (int i = 0; i < 10; ++i) {
        ASSERT_TRUE(copy.back() == 10 - i - 1);
        copy.pop();
    }

    // test original again
    for (int i = 0; i < 10; ++i) {
        ASSERT_TRUE(vec[i] == i);
    }

    for (int i = 0; i < 10; ++i) {
        ASSERT_TRUE(vec.front() == i);
        vec.erase(0);
    }
    ASSERT_TRUE(vec.empty());

    // sort
    for (int i = 0; i < 10; ++i) {
        vec.insert(0, i);
    }
    vec.sort();
    for (int i = 0; i < 10; ++i) {
        ASSERT_TRUE(vec[i] == i);
    }
}
void testVector1() { testVector<int>();     }
void testVector2() { testVector<Integer>(); }

template <class TYPE> void testHashTable() {
    const String KEYS = "abcdefghijklmnopqrstuvwxyz";
    HashTable<String, TYPE> table;
    const UInt32 tableLength = 8;

    ASSERT_EQ(table.size(), 0);
    ASSERT_TRUE(table.empty());

    // insert two tableLength => force grow
    for (UInt32 i = 0; i < tableLength * 2; ++i) {
        table.insert(String(KEYS[i]), i);
    }
    ASSERT_EQ(table.size(), tableLength * 2);
    ASSERT_FALSE(table.empty());

    // test access each key & value
    for (UInt32 i = 0; i < tableLength * 2; ++i) {
        ASSERT_TRUE(table.find(String(KEYS[i])) != Nil);
        ASSERT_TRUE(table[String(KEYS[i])] == i);
    }

    // test const access
    const HashTable<String, TYPE>& ctable = table;
    for (UInt32 i = 0; i < tableLength * 2; ++i) {
        ASSERT_TRUE(ctable.find(String(KEYS[i])) != Nil);
        ASSERT_TRUE(ctable[String(KEYS[i])] == i);
    }

    // test copy
    HashTable<String, TYPE> copy = table;
    for (UInt32 i = 0; i < tableLength * 2; ++i) {
        ASSERT_TRUE(copy.find(String(KEYS[i])) != Nil);
        ASSERT_TRUE(copy[String(KEYS[i])] == i);
    }
    
    // test copy operator
    copy = table;
    for (UInt32 i = 0; i < tableLength * 2; ++i) {
        ASSERT_TRUE(copy.find(String(KEYS[i])) != Nil);
        ASSERT_TRUE(copy[String(KEYS[i])] == i);
    }

    // test erase
    for (UInt32 i = 0; i < tableLength * 2; ++i) {
        ASSERT_EQ(copy.erase(String(KEYS[i])), 1);
    }
    ASSERT_TRUE(copy.empty());

    // test original table again
    for (UInt32 i = 0; i < tableLength * 2; ++i) {
        ASSERT_TRUE(table.find(String(KEYS[i])) != Nil);
        ASSERT_TRUE(table[String(KEYS[i])] == i);
    }
}

void testHashTable1() { testHashTable<int>();       }
void testHashTable2() { testHashTable<Integer>();   }

void testContent() {
    if (gCurrentDir == Nil) {
        ERROR("skip testContent");
        return;
    }
    String url = String::format("%s/file2", gCurrentDir);
    sp<Content> pipe = Content::Create(url);
    
    ASSERT_EQ(pipe->mode(), Protocol::Read);
    ASSERT_EQ(pipe->offset(), 0);
    ASSERT_EQ(pipe->capacity(), 1024*1024); // 1M
    
    sp<Buffer> data = pipe->readBytes(256);
    for (UInt32 i = 0; i < 256; ++i) {
        ASSERT_EQ((UInt8)data->data()[i], (UInt8)i);
    }
    ASSERT_EQ(pipe->offset(), 256);
    ASSERT_EQ(pipe->capacity(), 1024*1024); // 1M
    
    // seek test
    pipe->resetBytes();  // seek in cache
    ASSERT_EQ(pipe->offset(), 0);
    ASSERT_EQ(pipe->capacity(), 1024*1024); // 1M
    
    pipe->readBytes(256);
    pipe->resetBytes();
    pipe->skipBytes(1024);   // seek in cache @ 1k
    ASSERT_EQ(pipe->offset(), 1024);
    ASSERT_EQ(pipe->capacity(), 1024*1024); // 1M
    
    pipe->resetBytes();
    pipe->skipBytes(512*1024);   // seek @ 5k
    ASSERT_EQ(pipe->offset(), 512*1024);
    ASSERT_EQ(pipe->capacity(), 1024*1024); // 1M
}

#define TEST_ENTRY(FUNC)                    \
    TEST_F(MyTest, FUNC) {                  \
        INFO("Begin Test MyTest."#FUNC);    \
        MemoryAnalyzerPrepare(); {                 \
            FUNC();                         \
        } MemoryAnalyzerFinalize();                \
        INFO("End Test MyTest."#FUNC);      \
    }

TEST_ENTRY(testAtomic);
TEST_ENTRY(testSharedObject);
TEST_ENTRY(testAllocator);
TEST_ENTRY(testString);
TEST_ENTRY(testBits);
TEST_ENTRY(testBuffer);
TEST_ENTRY(testMessage);
TEST_ENTRY(testLooper);
TEST_ENTRY(testDispatchQueue);
TEST_ENTRY(testContent);
TEST_ENTRY(testQueue1);
TEST_ENTRY(testQueue2);
TEST_ENTRY(testList1);
TEST_ENTRY(testList2);
TEST_ENTRY(testVector1);
TEST_ENTRY(testVector2);
TEST_ENTRY(testHashTable1);
TEST_ENTRY(testHashTable2);

int main(int argc, Char **argv) {
    testing::InitGoogleTest(&argc, argv);

    if (argc > 1)
        gCurrentDir = argv[1];

    int result = RUN_ALL_TESTS();

    return result;
}


