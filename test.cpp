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
    bool operator<(const Integer& rhs) const    { return value < rhs.value;     }
    bool operator==(int i) const                { return value == i;            }
    bool operator==(const Integer& rhs) const   { return value == rhs.value;    }
    Integer& operator++()                       { ++value; return *this;        }   // pre-increment
    Integer  operator++(int)                    { return value++;               }   // post-increment
};

template <class TYPE> static bool LessCompare(const TYPE* lhs, const TYPE* rhs) {
    return *lhs < *rhs;
}

struct MySharedObject : public SharedObject {
    MySharedObject() { }
};

void testSharedObject() {
    SharedObject * shared = new MySharedObject;
    ASSERT_EQ(shared->GetRetainCount(), 0);

    Object<SharedObject> sp_shared1 = shared;
    ASSERT_EQ(shared->GetRetainCount(), 1);
    ASSERT_EQ(sp_shared1.refsCount(), 1);

    Object<SharedObject> sp_shared2 = sp_shared1;
    ASSERT_EQ(shared->GetRetainCount(), 2);
    ASSERT_EQ(sp_shared2.refsCount(), 2);

    sp_shared2.clear();
    ASSERT_EQ(shared->GetRetainCount(), 1);
    sp_shared1.clear();
}

void testAllocator() {
    Object<Allocator> allocator = kAllocatorDefault;
    void * p = allocator->allocate(1024);
    ASSERT_TRUE(p != NULL);

    p = allocator->reallocate(p, 2048);
    ASSERT_TRUE(p != NULL);

    allocator->deallocate(p);
}

void testString() {
    const char * STRING = "abcdefghijklmn";
    const String s0;
    const String s1 = STRING;
    const String s2 = "ABCDEFGHIJKLMN";
    const String s3 = "abcdefgabcdefg";
    const String s4 = STRING;
    const size_t n = strlen(STRING);

    // check on empty string
    ASSERT_EQ(s0.size(), 0);
    ASSERT_TRUE(s0.empty());
    ASSERT_EQ(s0[0], '\0');
    ASSERT_TRUE(s0.c_str() != NULL);
    ASSERT_STREQ(s0.c_str(), "");

    // check on non-empty string
    ASSERT_EQ(s1.size(), 14);
    ASSERT_FALSE(s1.empty());
    ASSERT_TRUE(s1.c_str() != NULL);
    ASSERT_STREQ(s1.c_str(), STRING);
    for (size_t i = 0; i < n; ++i) {
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
    ASSERT_TRUE(s1.equalsIgnoreCase(s2));
    ASSERT_EQ(s1.compareIgnoreCase(s2), 0);

    // indexOf
    ASSERT_EQ(s3.indexOf("a"), 0);
    ASSERT_EQ(s3.indexOf('a'), 0);
    ASSERT_EQ(s3.indexOf("b"), 1);
    ASSERT_EQ(s3.indexOf('b'), 1);
    ASSERT_EQ(s3.indexOf(7, "c"), 9);
    ASSERT_EQ(s3.indexOf(7, 'c'), 9);
    ASSERT_EQ(s3.lastIndexOf("a"), 7);
    ASSERT_EQ(s3.lastIndexOf('a'), 7);
    ASSERT_EQ(s3.lastIndexOf("b"), 8);
    ASSERT_EQ(s3.lastIndexOf('b'), 8);

    // start & end with
    ASSERT_TRUE(s1.startsWith("abc"));
    ASSERT_FALSE(s1.startsWith("aaa"));
    ASSERT_TRUE(s1.endsWith("lmn"));
    ASSERT_FALSE(s1.endsWith("aaa"));
    ASSERT_TRUE(s2.startsWithIgnoreCase("abc"));
    ASSERT_FALSE(s2.startsWithIgnoreCase("aaa"));
    ASSERT_TRUE(s2.endsWithIgnoreCase("lmn"));
    ASSERT_FALSE(s2.endsWithIgnoreCase("aaa"));

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
        tmp.replaceAll("abc", "cba");
        ASSERT_STREQ(tmp.c_str(), "cbadefgcbadefg");
        tmp = " abcdefghijklmn ";
        tmp.trim();
        ASSERT_TRUE(tmp == s1);
    }
}

void testBuffer() {
    Object<Buffer> buffer = new Buffer(128);
    ASSERT_EQ(buffer->type(), kBufferTypeDefault);
    ASSERT_EQ(buffer->capacity(), 128);
    ASSERT_EQ(buffer->empty(), 128);
    ASSERT_EQ(buffer->ready(), 0);
    ASSERT_EQ(buffer->size(), 0);
    
    buffer->write("abcdefgh");  // write 8 bytes
    ASSERT_EQ(buffer->capacity(), 128);
    ASSERT_EQ(buffer->empty(), 120);
    ASSERT_EQ(buffer->ready(), 8);
    
    buffer->reset();
    ASSERT_EQ(buffer->capacity(), 128);
    ASSERT_EQ(buffer->empty(), 128);
    ASSERT_EQ(buffer->ready(), 0);
    
    buffer->step(8);    // move write pos by 8 bytes
    ASSERT_EQ(buffer->capacity(), 128);
    ASSERT_EQ(buffer->empty(), 120);
    ASSERT_EQ(buffer->ready(), 8);
    
    buffer->skip(8);    // move read pos by 8 bytes
    ASSERT_EQ(buffer->capacity(), 128);
    ASSERT_EQ(buffer->empty(), 120);
    ASSERT_EQ(buffer->ready(), 0);
}

void testBitReader() {
    
}

void testBitWritter() {
    
}

struct EmptySharedObject : public SharedObject {
    EmptySharedObject() : SharedObject() { }
    virtual ~EmptySharedObject() { }
};

void testMessage() {
    Message message;
    SharedObject * shared = new EmptySharedObject;
    
#define TEST_MESSAGE(NAME, VALUE)                       \
    ASSERT_FALSE(message.contains(#NAME));              \
    message.set##NAME(#NAME, VALUE);                    \
    ASSERT_EQ(message.find##NAME(#NAME), VALUE);        \
    ASSERT_TRUE(message.contains(#NAME));               \

    TEST_MESSAGE(Int32, 32)
    TEST_MESSAGE(Int64, 64)
    TEST_MESSAGE(Float, 1.0)
    TEST_MESSAGE(Double, 2.0)
    TEST_MESSAGE(Pointer, &message);
    TEST_MESSAGE(Object, shared)
    
#undef TEST_MESSAGE
    
    const char * string = "abcdefg";
    message.setString("String", string);
    ASSERT_TRUE(message.contains("String"));
    ASSERT_STREQ(message.findString("String"), string);
    
    Integer Int(1);
    message.set<Integer>("Integer", Int);
    ASSERT_TRUE(message.contains("Integer"));
    ASSERT_EQ(message.find<Integer>("Integer"), Int);
}

struct ThreadRunnable : public Runnable {
    const String name;
    ThreadRunnable(const String& _name) : name(_name) { }
    virtual void run() {
        INFO("ThreadRunnable %s", name.c_str());
    }
};

struct ThreadSyncRunnable : public SyncRunnable {
    virtual void sync() {
        INFO("ThreadSyncRunnable");
    }
};

void testThread() {
    // easy way
    Thread(new ThreadRunnable("Thread 0")).detach();    // detach without run
    Thread(new ThreadRunnable("Thread 0")).join();      // join without run
    Thread(new ThreadRunnable("Thread 1")).run().detach();
    Thread(new ThreadRunnable("Thread 2")).run().join();
    
    // thread type
    Thread(new ThreadRunnable("Lowest Thread"), kThreadLowest).run().join();
    Thread(new ThreadRunnable("Backgroud Thread"), kThreadBackgroud).run().join();
    Thread(new ThreadRunnable("Normal Thread"), kThreadNormal).run().join();
    Thread(new ThreadRunnable("Foregroud Thread"), kThreadForegroud).run().join();
    Thread(new ThreadRunnable("System Thread"), kThreadSystem).run().join();
    Thread(new ThreadRunnable("Kernel Thread"), kThreadKernel).run().join();
    Thread(new ThreadRunnable("Realtime Thread"), kThreadRealtime).run().join();
    Thread(new ThreadRunnable("Highest Thread"), kThreadHighest).run().join();

    
    // hard way
    Thread thread(new ThreadRunnable("Thread 3"));
    ASSERT_TRUE(thread.joinable());
    thread.setName("Thread 3").setType(kThreadNormal);
    ASSERT_STREQ("Thread 3", thread.name().c_str());
    ASSERT_EQ(kThreadNormal, thread.type());
    ASSERT_EQ(Thread::kThreadInitializing, thread.state());
    thread.run();
    ASSERT_EQ(Thread::kThreadRunning, thread.state());
    ASSERT_TRUE(thread.joinable());
    thread.join();
    ASSERT_FALSE(thread.joinable());
    ASSERT_EQ(Thread::kThreadTerminated, thread.state());
    
    // sync
    Object<SyncRunnable> sync = new ThreadSyncRunnable;
    Thread(sync).run().detach();
    SleepTimeMs(100);
    sync->wait();
    
    // static members
    
}

struct MainLooperAssist : public Runnable {
    virtual void run() {
        INFO("post prepare");
        SleepTimeMs(1000); // 1s
        Object<Looper> main = Looper::Main();
        
        INFO("post assist 0");
        main->post(new ThreadRunnable("assist 0"));
        
        INFO("post assist 1");
        main->post(new ThreadRunnable("assist 1"), 500000LL);   // 500ms
        SleepTimeMs(10);
        INFO("post assist 2");
        main->post(new ThreadRunnable("assist 2"));
        
        SleepTimeMs(500);
        main->flush();
        
        main->terminate();
    }
};

void testLooper() {
    Object<Looper> looper1 = Looper::Create("Looper 1");
    looper1->loop();
    looper1->post(new ThreadRunnable("Looper run 1"));
    looper1->terminate(true);
    looper1.clear();

    Object<Looper> current = Looper::Current();
    ASSERT_TRUE(current == NULL);

    Object<Looper> main = Looper::Main();
    Object<Looper> assist = Looper::Create("assist");
    assist->post(new MainLooperAssist);
    assist->loop();
    ASSERT_TRUE(main != NULL);
    main->loop();
    main->terminate();
    assist->terminate();
}

template <class TYPE> struct QueueConsumer : public Runnable {
    LockFree::Queue<TYPE> mQueue;
    const int kCount;
    QueueConsumer() : Runnable(), kCount(10000) { }
    
    virtual void run() {
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
    Object<QueueConsumer<TYPE> > consumer = new QueueConsumer<TYPE>();
    Thread thread(consumer);
    thread.run();
    
    // producer
    for (TYPE i = 0; i < consumer->kCount; ++i) {
        consumer->mQueue.push(i);
    }
    
    thread.join();
    ASSERT_TRUE(consumer->mQueue.empty());
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
    const size_t tableLength = 8;

    ASSERT_EQ(table.size(), 0);
    ASSERT_TRUE(table.empty());

    // insert two tableLength => force grow
    for (size_t i = 0; i < tableLength * 2; ++i) {
        table.insert(String(KEYS[i]), i);
    }
    ASSERT_EQ(table.size(), tableLength * 2);
    ASSERT_FALSE(table.empty());

    // test access each key & value
    for (size_t i = 0; i < tableLength * 2; ++i) {
        ASSERT_TRUE(table.find(String(KEYS[i])) != NULL);
        ASSERT_TRUE(table[String(KEYS[i])] == i);
    }

    // test const access
    const HashTable<String, TYPE>& ctable = table;
    for (size_t i = 0; i < tableLength * 2; ++i) {
        ASSERT_TRUE(ctable.find(String(KEYS[i])) != NULL);
        ASSERT_TRUE(ctable[String(KEYS[i])] == i);
    }

    // test copy
    HashTable<String, TYPE> copy = table;
    for (size_t i = 0; i < tableLength * 2; ++i) {
        ASSERT_TRUE(copy.find(String(KEYS[i])) != NULL);
        ASSERT_TRUE(copy[String(KEYS[i])] == i);
    }
    
    // test copy operator
    copy = table;
    for (size_t i = 0; i < tableLength * 2; ++i) {
        ASSERT_TRUE(copy.find(String(KEYS[i])) != NULL);
        ASSERT_TRUE(copy[String(KEYS[i])] == i);
    }

    // test erase
    for (size_t i = 0; i < tableLength * 2; ++i) {
        ASSERT_EQ(copy.erase(String(KEYS[i])), 1);
    }
    ASSERT_TRUE(copy.empty());

    // test original table again
    for (size_t i = 0; i < tableLength * 2; ++i) {
        ASSERT_TRUE(table.find(String(KEYS[i])) != NULL);
        ASSERT_TRUE(table[String(KEYS[i])] == i);
    }
}

void testHashTable1() { testHashTable<int>();       }
void testHashTable2() { testHashTable<Integer>();   }

extern "C" void malloc_prepare();
extern "C" void malloc_bypass();
extern "C" void malloc_finalize();
#define TEST_ENTRY(FUNC)                    \
    TEST_F(MyTest, FUNC) {                  \
        INFO("Begin Test MyTest."#FUNC);    \
        malloc_prepare(); {                 \
            FUNC();                         \
        } malloc_finalize();                \
        INFO("End Test MyTest."#FUNC);      \
    }

TEST_ENTRY(testSharedObject);
TEST_ENTRY(testAllocator);
TEST_ENTRY(testQueue1);
TEST_ENTRY(testQueue2);
TEST_ENTRY(testList1);
TEST_ENTRY(testList2);
TEST_ENTRY(testVector1);
TEST_ENTRY(testVector2);
TEST_ENTRY(testHashTable1);
TEST_ENTRY(testHashTable2);
TEST_ENTRY(testString);
TEST_ENTRY(testBuffer);
TEST_ENTRY(testMessage);
TEST_ENTRY(testThread);
TEST_ENTRY(testLooper);

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);

    int result = RUN_ALL_TESTS();

    return result;
}


