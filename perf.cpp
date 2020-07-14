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


// File:    test.cpp
// Author:  mtdcy.chen
// Changes:
//          1. 20160701     initial version

#define REMOVE_ATOMICS
#define LOG_TAG "perf"
#include <ABE/ABE.h>

#include <list>     // std::list
#include <vector>   // std::vector
#if defined(__APPLE__)
#include <unordered_map>    // std::unordered_map
#endif
USING_NAMESPACE_ABE

using namespace std;

#define PERF_TEST_COUNT     1000000
#define PERF_PRODUCER       3
#define LOOPER_TEST_COUNT   1000
#define LOOPER_TEST_SLEEP   1000  // 1ms

#define MULTI_THREAD 1

struct Integer {
    int value;
    Integer() : value(0) { }
    Integer(int i) : value(i) { }
    Bool operator<(const Integer& rhs) const { return value < rhs.value; }
    Bool operator==(const Integer& rhs) const { return value == rhs.value; }
    Bool operator==(int rhs) const { return value == rhs; }
};

struct QueueSingleConsumer : public Job {
    LockFree::Queue<Integer>    mQueue;
    int                         mNext;
    QueueSingleConsumer() : mNext(0) { }
    virtual void onJob() {
        Int64 now = SystemTimeUs();
        for (;;) {
            Integer i;
            if (mQueue.pop(i)) {
                CHECK_EQ(i.value, mNext++);
                if (i.value == PERF_TEST_COUNT) {
                    break;
                }
            } else {
                //Thread::Sleep(1);    // 1us
            }
        }
        Int64 delta = SystemTimeUs() - now;
        INFO("Queue pop() test takes %" PRId64 " us, each %.3f us", delta, (Float64)delta / PERF_TEST_COUNT);
    }
};

struct QueueProducer : public Job {
    LockFree::Queue<Integer>    mQueue;
    volatile int                mNext;
    QueueProducer() : mNext(0) { }
    virtual void onJob() {
        Int64 now = SystemTimeUs();
        for (int i = 0; i <= PERF_TEST_COUNT; ++i) {
            mQueue.push(__atomic_fetch_add(&mNext, 1, __ATOMIC_SEQ_CST));
        }
        Int64 delta = SystemTimeUs() - now;
        INFO("Queue push() test takes %" PRId64 " us, each %.3f us", delta, (Float64)delta / PERF_TEST_COUNT);
    }
};

void QueuePerf() {
    INFO("Queue push() | pop()");
    Int64 now, delta;
    LockFree::Queue<int> queue;
    now = SystemTimeUs();
    for (int i = 0; i <= PERF_TEST_COUNT; ++i) {
        queue.push(i);
    }
    delta = SystemTimeUs() - now;
    INFO("Queue push() test takes %" PRId64 " us, each %.3f us", delta, (Float64)delta / PERF_TEST_COUNT);
    
    now = SystemTimeUs();
    for (int i = 0; i <= PERF_TEST_COUNT; ++i) {
        int tmp;
        CHECK_TRUE(queue.pop(tmp));
        CHECK_EQ(tmp, i);
    }
    delta = SystemTimeUs() - now;
    INFO("Queue pop() test takes %" PRId64 " us, each %.3f us", delta, (Float64)delta / PERF_TEST_COUNT);
    INFO("---");
    
#if MULTI_THREAD
    // single producer & single consumer test
    INFO("Queue single producer & single consumer");
    sp<QueueSingleConsumer> consumer = new QueueSingleConsumer;
    Thread thread(consumer);
    thread.run();
    now = SystemTimeUs();
    for (int i = 0; i <= PERF_TEST_COUNT; ++i) {
        consumer->mQueue.push(i);
    }
    delta = SystemTimeUs() - now;
    INFO("Queue push() test takes %" PRId64 " us, each %.3f us", delta, (Float64)delta / PERF_TEST_COUNT);
    thread.join();
    INFO("---");

    // multi producer & single consumer test
    INFO("Queue multi producer & single consumer");
    sp<QueueProducer> producer = new QueueProducer;
    Vector<Thread> threads;
    for (UInt32 i = 0; i < PERF_PRODUCER; ++i) threads.push(Thread(producer));
    for (UInt32 i = 0; i < PERF_PRODUCER; ++i) threads[i].run();
    
    now = SystemTimeUs();
    for (;;) {
        Integer tmp;
        if (producer->mQueue.pop(tmp)) {
            if (tmp.value == PERF_TEST_COUNT * PERF_PRODUCER) break;
        } else {
            // NOTHING
        }
    }
    delta = SystemTimeUs() - now;
    INFO("Queue pop() test takes %" PRId64 " us, each %.3f us", delta, (Float64)delta / PERF_TEST_COUNT);
    for (UInt32 i = 0; i < PERF_PRODUCER; ++i) {
        threads[i].join();
    }
    threads.clear();
    INFO("---");
#endif
}

// compare to LockFree::Queue
struct ListConsumer : public Job {
    Mutex           mLock;
    List<Integer>   mList;
    int             mNext;
    ListConsumer() : mNext(0) { }
    virtual void onJob() {
        Int64 now = SystemTimeUs();
        for (;;) {
            AutoLock _l(mLock);
            if (mList.size()) {
                Integer i = mList.front();
                mList.pop();
                CHECK_EQ(i.value, mNext++);
                if (i.value == PERF_TEST_COUNT) {
                    break;
                }
            } else {
                //Thread::Sleep(1);    // 1us
            }
        }
        Int64 delta = SystemTimeUs() - now;
        INFO("List pop() test takes %" PRId64 " us, each %.3f us", delta, (Float64)delta / PERF_TEST_COUNT);
    }
};

struct ListProducer : public Job {
    Mutex           mLock;
    List<Integer>   mList;
    int             mNext;
    ListProducer() : mNext(0) { }
    virtual void onJob() {
        Int64 now = SystemTimeUs();
        for (int i = 0; i <= PERF_TEST_COUNT; ++i) {
            AutoLock _l(mLock);
            mList.push(mNext++);
        }
        Int64 delta = SystemTimeUs() - now;
        INFO("List push() test takes %" PRId64 " us, each %.3f us", delta, (Float64)delta / PERF_TEST_COUNT);
    }
};

void ListPerf() {
    INFO("List push() | pop()");
    Int64 now, delta;
    List<int> list;
    now = SystemTimeUs();
    for (int i = 0; i <= PERF_TEST_COUNT; ++i) { list.push(i); }
    delta = SystemTimeUs() - now;
    INFO("List push() test takes %" PRId64 " us, each %.3f us", delta, (Float64)delta / PERF_TEST_COUNT);
    
    now = SystemTimeUs();
    list.sort();
    delta = SystemTimeUs() - now;
    INFO("List sort() test takes %" PRId64 " us, each %.3f us", delta, (Float64)delta / PERF_TEST_COUNT);
    
    now = SystemTimeUs();
    for (int i = 0; i <= PERF_TEST_COUNT; ++i) {
        CHECK_EQ(list.front(), i);
        list.pop();
    }
    delta = SystemTimeUs() - now;
    INFO("List pop() test takes %" PRId64 " us, each %.3f us", delta, (Float64)delta / PERF_TEST_COUNT);
    
    CHECK_TRUE(list.empty());
    for (int i = 0; i <= PERF_TEST_COUNT - i; ++i) {
        list.push(i);
        if (i < PERF_TEST_COUNT - i) list.push(PERF_TEST_COUNT - i);
    }
    now = SystemTimeUs();
    list.sort();
    delta = SystemTimeUs() - now;
    INFO("List sort() test takes %" PRId64 " us, each %.3f us", delta, (Float64)delta / PERF_TEST_COUNT);
    
    INFO("---");
    
#if MULTI_THREAD
    // compare to LockFree::Queue
    INFO("List single producer & single consumer");
    sp<ListConsumer> consumer = new ListConsumer;
    Thread thread(consumer);
    thread.run();
    now = SystemTimeUs();
    for (int i = 0; i <= PERF_TEST_COUNT; ++i) {
        AutoLock _l(consumer->mLock);
        consumer->mList.push(i);
    }
    delta = SystemTimeUs() - now;
    INFO("List push() test takes %" PRId64 " us, each %.3f us", delta, (Float64)delta / PERF_TEST_COUNT);
    thread.join();
    INFO("---");

    INFO("List multi producer & single consumer");
    sp<ListProducer> producer = new ListProducer;
    Vector<Thread> threads;
    for (UInt32 i = 0; i < PERF_PRODUCER; ++i) threads.push(Thread(producer));
    for (UInt32 i = 0; i < PERF_PRODUCER; ++i) threads[i].run();
    
    now = SystemTimeUs();
    for (;;) {
        AutoLock _l(producer->mLock);
        if (producer->mList.size()) {
            Integer tmp = producer->mList.front();
            producer->mList.pop();
            if (tmp.value == PERF_TEST_COUNT * PERF_PRODUCER) break;
        } else {
            // NOTHING
        }
    }
    delta = SystemTimeUs() - now;
    INFO("List pop() test takes %" PRId64 " us, each %.3f us", delta, (Float64)delta / (PERF_TEST_COUNT * PERF_PRODUCER));
    for (UInt32 i = 0; i < PERF_PRODUCER; ++i) {
        threads[i].join();
    }
    threads.clear();
    INFO("---");
#endif
}

void STDListPerf() {
    INFO("std::list push() | pop()");
    Int64 now, delta;
    Float64 each;
    list<int> list;
    now = SystemTimeUs();
    for (int i = 0; i <= PERF_TEST_COUNT; ++i) { list.push_back(i); }
    delta = SystemTimeUs() - now;
    each = (Float64)delta / PERF_TEST_COUNT;
    INFO("std::list push() test takes %" PRId64 " us, each %.3f us", delta, each);
    
    now = SystemTimeUs();
    list.sort();
    delta = SystemTimeUs() - now;
    INFO("std::list sort() test takes %" PRId64 " us, each %.3f us", delta, (Float64)delta / PERF_TEST_COUNT);
    
    now = SystemTimeUs();
    for (int i = 0; i <= PERF_TEST_COUNT; ++i) {
        CHECK_EQ(list.front(), i);
        list.pop_front();
    }
    delta = SystemTimeUs() - now;
    each = (Float64)delta / PERF_TEST_COUNT;
    INFO("std::list pop() test takes %" PRId64 " us, each %.3f us", delta, each);
    
    CHECK_TRUE(list.empty());
    for (int i = 0; i <= PERF_TEST_COUNT - i; ++i) {
        list.push_back(i);
        if (i < PERF_TEST_COUNT - i) list.push_back(PERF_TEST_COUNT - i);
    }
    now = SystemTimeUs();
    list.sort();
    delta = SystemTimeUs() - now;
    INFO("std::list sort() test takes %" PRId64 " us, each %.3f us", delta, (Float64)delta / PERF_TEST_COUNT);
    INFO("---");
}

// FIXME:
// 1. why Vector::push() is better than List::push() a lot
//      Vector::push() only takes half time of List::push()
// 2. Vector is bad than std::vector
// 3. Vector::push(const TYPE&) is slower than Vector::push(), at least for builtin type, why???
template <class TYPE> void VectorPerfInt() {
    Int64 now, delta;
    Float64 each;
    INFO("Vector push() | pop()");
    now = SystemTimeUs();
    Vector<TYPE> vec;
    for (int i = 0; i <= PERF_TEST_COUNT; ++i) { vec.push(i); }
    delta = SystemTimeUs() - now;
    each = (Float64)delta / PERF_TEST_COUNT;
    INFO("Vector push() test takes %" PRId64 " us, each %.3f us", delta, each);
    
    now = SystemTimeUs();
    vec.sort();
    delta = SystemTimeUs() - now;
    each = (Float64)delta / PERF_TEST_COUNT;
    INFO("Vector sort() test takes %" PRId64 " us, each %.3f us", delta, each);
    
    now = SystemTimeUs();
    for (int i = 0; i <= PERF_TEST_COUNT; ++i) { CHECK_TRUE(vec[i] == i); }
    delta = SystemTimeUs() - now;
    each = (Float64)delta / PERF_TEST_COUNT;
    INFO("Vector operator[] test takes %" PRId64 " us, each %.3f us", delta, each);
    
    now = SystemTimeUs();
    for (int i = 0; i <= PERF_TEST_COUNT; ++i) { vec.pop(); }
    delta = SystemTimeUs() - now;
    each = (Float64)delta / PERF_TEST_COUNT;
    INFO("Vector pop() test takes %" PRId64 " us, each %.3f us", delta, each);
    
    CHECK_TRUE(vec.empty());
    for (int i = 0; i <= PERF_TEST_COUNT - i; ++i) {
        vec.push(i);
        if (i < PERF_TEST_COUNT - i) vec.push(PERF_TEST_COUNT - i);
    }
    now = SystemTimeUs();
    vec.sort();
    delta = SystemTimeUs() - now;
    each = (Float64)delta / PERF_TEST_COUNT;
    INFO("Vector sort() test takes %" PRId64 " us, each %.3f us", delta, each);
    for (int i = 0; i <= PERF_TEST_COUNT; ++i) { CHECK_TRUE(vec[i] == i); }

    INFO("---");
}

void VectorPerf() {
    INFO("Vector with builtin type");
    VectorPerfInt<int>();
    INFO("Vector with object type");
    VectorPerfInt<Integer>();
}

template <class TYPE> void STDVectorPerfInt() {
    Int64 now, delta;
    Float64 each;
    INFO("std::vector push() | pop()");
    now = SystemTimeUs();
    vector<TYPE> vec;
    for (int i = 0; i <= PERF_TEST_COUNT; ++i) { vec.push_back(i); }
    delta = SystemTimeUs() - now;
    each = (Float64)delta / PERF_TEST_COUNT;
    INFO("std::vector push_back() test takes %" PRId64 " us, each %.3f us", delta, each);
    
    now = SystemTimeUs();
    for (int i = 0; i <= PERF_TEST_COUNT; ++i) { CHECK_TRUE(vec[i] == i); }
    delta = SystemTimeUs() - now;
    each = (Float64)delta / PERF_TEST_COUNT;
    INFO("std::vector operator[] test takes %" PRId64 " us, each %.3f us", delta, each);
    
    now = SystemTimeUs();
    for (int i = 0; i <= PERF_TEST_COUNT; ++i) { vec.pop_back(); }
    delta = SystemTimeUs() - now;
    each = (Float64)delta / PERF_TEST_COUNT;
    INFO("std::vector pop_back() test takes %" PRId64 " us, each %.3f us", delta, each);
    INFO("---");
}

void STDVectorPerf() {
    INFO("std::vector with builtin type");
    STDVectorPerfInt<int>();
    INFO("std::vector with object type");
    STDVectorPerfInt<Integer>();
}

void HashTablePerf() {
    Int64 now, delta;
    Float64 each;
    INFO("HashTable insert() | erase()");
    now = SystemTimeUs();
    HashTable<int, int> hashtable;
    for (int i = 0; i <= PERF_TEST_COUNT; ++i) {
        hashtable.insert(i, i);
    }
    delta = SystemTimeUs() - now;
    each = (Float64)delta / PERF_TEST_COUNT;
    INFO("HashTable insert() test takes %" PRId64 " us, each %.3f us", delta, each);
    
    now = SystemTimeUs();
    for (int i = 0; i <= PERF_TEST_COUNT; ++i) {
        CHECK_EQ(hashtable[i], i);
    }
    delta = SystemTimeUs() - now;
    each = (Float64)delta / PERF_TEST_COUNT;
    INFO("HashTable operator[] test takes %" PRId64 " us, each %.3f us", delta, each);

    now = SystemTimeUs();
    for (int i = 0; i <= PERF_TEST_COUNT; ++i) {
        hashtable.erase(i);
    }
    delta = SystemTimeUs() - now;
    each = (Float64)delta / PERF_TEST_COUNT;
    INFO("HashTable erase() test takes %" PRId64 " us, each %.3f us", delta, each);
    INFO("---");
}

#if defined(__APPLE__)
void STDHashTablePerf() {
    Int64 now, delta;
    Float64 each;
    INFO("std::unordered_map insert() | erase()");
    now = SystemTimeUs();
    unordered_map<int, int> hashtable;
    for (int i = 0; i <= PERF_TEST_COUNT; ++i) {
        hashtable.insert(std::make_pair(i, i));
    }
    delta = SystemTimeUs() - now;
    each = (Float64)delta / PERF_TEST_COUNT;
    INFO("std::unordered_map insert() test takes %" PRId64 " us, each %.3f us", delta, each);
    
    now = SystemTimeUs();
    for (int i = 0; i <= PERF_TEST_COUNT; ++i) {
        CHECK_EQ(hashtable[i], i);
    }
    delta = SystemTimeUs() - now;
    each = (Float64)delta / PERF_TEST_COUNT;
    INFO("std::unordered_map operator[] test takes %" PRId64 " us, each %.3f us", delta, each);
    
    now = SystemTimeUs();
    for (int i = 0; i <= PERF_TEST_COUNT; ++i) {
        hashtable.erase(i);
    }
    delta = SystemTimeUs() - now;
    each = (Float64)delta / PERF_TEST_COUNT;
    INFO("std::unordered_map erase() test takes %" PRId64 " us, each %.3f us", delta, each);
    INFO("---");
}
#endif

struct EmptyJob : public Job {
    volatile int count;
    virtual void onJob() {
        SleepTimeUs(LOOPER_TEST_SLEEP);
        ++count;
    }
};

// FIXME:
// performance test shows:
//  Thread::Sleep is inaccurate at least on macOS with ~30% overhead
void LooperPerf() {
    Int64 now, delta;
    Float64 each;
    now = SystemTimeUs();
    for (UInt32 i = 0; i < LOOPER_TEST_COUNT; ++i) {
        SleepTimeUs(LOOPER_TEST_SLEEP);
    }
    delta = SystemTimeUs() - now;
    each = (Float64)delta / LOOPER_TEST_COUNT;
    INFO("sleep() takes %" PRId64 " us, each %.3f us, overhead %.3f", delta, each, each / LOOPER_TEST_SLEEP - 1);
    
    now = SystemTimeUs();
    Mutex lock; Condition wait;
    for (UInt32 i = 0; i < LOOPER_TEST_COUNT; ++i) {
        AutoLock _l(lock);
        wait.waitRelative(lock, LOOPER_TEST_SLEEP * 1000LL);
    }
    delta = SystemTimeUs() - now;
    each = (Float64)delta / LOOPER_TEST_COUNT;
    INFO("Condition.waitRelative() takes %" PRId64 " us, each %.3f us, overhead %.3f", delta, each, each / LOOPER_TEST_SLEEP - 1);
    
#if 0
    now = SystemTimeUs();
    for (UInt32 i = 0; i < LOOPER_TEST_COUNT; ++i) {
        sp<Job> r = new EmptyJob;
        r->run();
    }
    delta = SystemTimeUs() - now;
    each = (Float64)delta / LOOPER_TEST_COUNT;
    INFO("Job::run() takes %" PRId64 " us, each %.3f us, overhead %.3f", delta, each, each / LOOPER_TEST_SLEEP - 1);
#endif
    
    sp<Looper> looper = new Looper("LooperPerf");
    sp<EmptyJob> routine = new EmptyJob;
    routine->count = 0;
    now = SystemTimeUs();
    for (UInt32 i = 0; i < LOOPER_TEST_COUNT; ++i) {
        looper->post(routine);
    }
    looper.clear();     // wait jobs complete
    CHECK_EQ(routine->count, LOOPER_TEST_COUNT);
    delta = SystemTimeUs() - now;
    each = (Float64)delta / LOOPER_TEST_COUNT;
    INFO("Looper() takes %" PRId64 " us, each %.3f us, overhead %.3f", delta, each, each / LOOPER_TEST_SLEEP - 1);
    
    now = SystemTimeUs();
    for (UInt32 i = 0; i < LOOPER_TEST_COUNT; ++i) {
        Thread(new EmptyJob).run().join();
    }
    delta = SystemTimeUs() - now;
    each = (Float64)delta / LOOPER_TEST_COUNT;
    INFO("Thread() takes %" PRId64 " us, each %.3f us, overhead %.3f", delta, each, each / LOOPER_TEST_SLEEP - 1);
    
    sp<DispatchQueue> queue = new DispatchQueue(new Looper("disp"));
    now = SystemTimeUs();
    for (UInt32 i = 0; i < LOOPER_TEST_COUNT; ++i) {
        queue->sync(new EmptyJob);
    }
    delta = SystemTimeUs() - now;
    each = (Float64)delta / LOOPER_TEST_COUNT;
    INFO("DispatchQueue() takes %" PRId64 " us, each %.3f us, overhead %.3f", delta, each, each / LOOPER_TEST_SLEEP - 1);
    
}

int main(int argc, Char ** argv) {

    QueuePerf();
    ListPerf();
    STDListPerf();
    VectorPerf();
    STDVectorPerf();
    HashTablePerf();
#if defined(__APPLE__)
    STDHashTablePerf();
#endif    
    LooperPerf();

    return 0;
}
