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


// File:    malloc.c
// Author:  mtdcy.chen
// Changes: 
//          1. 20161012     initial version
//
// leak detection is very useful for c/c++ development. 
// simply override malloc and free is a very simple implementation, but 
// it is sufficient for internal use as a quick detection. you can also
// use valgrind to check other memory errors.
//
// FIXME: 
// 1. the g_malloc_info will NOT not be freed.

#define _DARWIN_C_SOURCE    // for RTLD_NEXT
//#define _GNU_SOURCE         // for RTLD_NEXT on Linux

#define LOG_TAG  "debug.malloc"
//#define LOG_NDEBUG 0
#include "core/Log.h"

#include "core/Types.h"
#include "core/System.h"

#define MALLOC_CALLSTACK

#include <string.h>
#include <pthread.h>
#include <dlfcn.h>

#include "stl/HashTable.h"

// we can not use CHECK_EQ/... here, which use String
#define FATAL_CHECK(a, b, op) do {                                  \
    if (__builtin_expect(!((a) op (b)), 0)) {                       \
        FATAL(__FILE__ ":" __LITERAL_TO_STRING(__LINE__)              \
                " CHECK(" #a " " #op " " #b ") failed. ");          \
    }                                                               \
} while(0)

#define FATAL_CHECK_EQ(a, b)    FATAL_CHECK(a, b, ==)
#define FATAL_CHECK_GE(a, b)    FATAL_CHECK(a, b, >=)
#define FATAL_CHECK_GT(a, b)    FATAL_CHECK(a, b, >)

__BEGIN_NAMESPACE_ABE

#ifdef __GLIBC__
#define USING_DLSYM     0
#else
#define USING_DLSYM     1
#endif

typedef void*   (*real_malloc_t)(UInt32 n);
typedef void    (*real_free_t)(void *p);
typedef void*   (*real_realloc_t)(void *p, UInt32 n);
typedef Int     (*real_posix_memalign_t)(void **, UInt32, UInt32);
#if USING_DLSYM == 0
// http://stackoverflow.com/questions/5223971/question-about-overriding-c-standard-library-functions-and-how-to-link-everythin
//extern void *__libc_malloc(UInt32);
//extern void  __libc_free(void *);
//extern void *__libc_realloc(void *, UInt32);
extern __typeof (malloc) __libc_malloc;
extern __typeof (free) __libc_free;
extern __typeof (realloc) __libc_realloc;
#endif

// can NOT garentee static global variable initialization order,
// so put it inside function as static local variable.
static ABE_INLINE void * malloc_impl_malloc(UInt32 size) {
#if USING_DLSYM == 0
    static real_malloc_t _real_malloc = __libc_malloc;
#else
    static real_malloc_t _real_malloc = Nil;
    if (__builtin_expect(_real_malloc == Nil, False)) {
        DEBUG("initial malloc_impl_malloc");
        _real_malloc = (real_malloc_t)dlsym(RTLD_NEXT, "malloc");
    }
#endif
    return _real_malloc(size);
}

static ABE_INLINE void malloc_impl_free(void *ptr) {
#if USING_DLSYM == 0
    static real_free_t _real_free = __libc_free;
#else
    static real_free_t _real_free = Nil;
    if (__builtin_expect(_real_free == Nil, False)) {
        DEBUG("initial malloc_impl_free");
        _real_free = (real_free_t)dlsym(RTLD_NEXT, "free");
    }
#endif
    return _real_free(ptr);
}

static ABE_INLINE void * malloc_impl_realloc(void *ptr, UInt32 size) {
#if USING_DLSYM == 0
    static real_realloc_t _real_realloc = __libc_realloc;
#else
    static real_realloc_t _real_realloc = Nil;
#endif
    if (__builtin_expect(_real_realloc == Nil, False)) {
        DEBUG("initial malloc_impl_realloc");
        _real_realloc = (real_realloc_t)dlsym(RTLD_NEXT, "realloc");
    }
    return _real_realloc(ptr, size);
}

static ABE_INLINE Int malloc_impl_posix_memalign(void **memptr, UInt32 alignment, UInt32 n) {
    static real_posix_memalign_t _real_posix_memalign = Nil;
    if (__builtin_expect(_real_posix_memalign == Nil, False)) {
        DEBUG("initial malloc_impl_posix_memalign");
        _real_posix_memalign = (real_posix_memalign_t)dlsym(RTLD_NEXT, "posix_memalign");
    }
    return _real_posix_memalign(memptr, alignment, n);
}

struct RealAllocator : public Allocator {
    virtual void *  allocate(UInt32 size) { return malloc_impl_malloc(size); }
    virtual void *  reallocate(void * ptr, UInt32 size) { return malloc_impl_realloc(ptr, size); }
    virtual void    deallocate(void * ptr) { return malloc_impl_free(ptr); }
};

struct MallocBlock {
    UInt32          magic;
    UInt32          n;
    UInt32          flags;
    UInt32          stack_size;
    UInt64          stack[16];
    void *          real;
    void *          dummy2;
};

static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;
static HashTable<void *, MallocBlock *> g_blocks(8192, new RealAllocator);

static void printBlocks() {
    pthread_mutex_lock(&g_lock);
    if (g_blocks.empty()) {
        pthread_mutex_unlock(&g_lock);
        return;
    }
    UInt32 n = 0;
    UInt32 total = 0;
    HashTable<void *, MallocBlock*>::const_iterator it = g_blocks.cbegin();
    INFO("===============================================================");
    INFO("== current malloc info: ");
    for (; it != g_blocks.cend(); ++it) {
        const MallocBlock* block = it.value();
        INFO("== %p - %" PRIu32, block->real, block->n);
        CallStackPut(block->stack, block->stack_size);
        ++n;
        total += block->n;
    }
    INFO("== total %u bytes(%u)", total, n);
    INFO("===============================================================");
    pthread_mutex_unlock(&g_lock);
}

static MallocBlock* validateBlock(void *p) {
    CHECK_NULL(p);

    pthread_mutex_lock(&g_lock);
    MallocBlock * block = *g_blocks.find(p);
    pthread_mutex_unlock(&g_lock);

    if (block == Nil) return Nil;

    const UInt32 length = sizeof(struct MallocBlock) + block->n + sizeof(UInt32);
    FATAL_CHECK_EQ(block->magic, 0xbaaddead);
    UInt32 magic = ((UInt32*)((Char*)block + length))[-1];
    FATAL_CHECK_EQ(magic, 0xdeadbaad);
    return block;
}

//////////////////////////////////////////////////////////////////////////////
static void* malloc_impl_malloc_debug(UInt32 n) {
    FATAL_CHECK_GT(n, 0);
    UInt32 length = sizeof(MallocBlock) + n + sizeof(UInt32);
    MallocBlock * block = (MallocBlock*)malloc_impl_malloc(length);
    block->magic        = 0xbaaddead;
    block->n            = n;
    block->flags        = 0;
    block->real         = &block[1];
    block->stack_size   = CallStackGet(block->stack, 16);
    ((UInt32*)((Char*)block + length))[-1] = 0xdeadbaad;

    DEBUG("malloc %p@%p[%zu]", block->real, block, block->n);

    CHECK_NULL(block);
    pthread_mutex_lock(&g_lock);
    g_blocks.insert(block->real, block);
    pthread_mutex_unlock(&g_lock);
    return block->real;
}

static void  malloc_impl_free_debug(void *p) {
    MallocBlock *block = validateBlock(p);

    if (block == Nil) {
        // this should NOT happen if others' code are written in a proper way,
        // but no one can garentee that, like backtrace_symbols
        // XXX: malloc a block inside library, and ask someone else to free it 
        // is not a good program manner, even simply using free().
#if LOG_NDEBUG == 0
        DEBUG("free unregisterred pointer %p", p);
        CallStackPrint();
#endif
        return malloc_impl_free(p);
    }
    DEBUG("free %p@%p[%zu]", block->real, block, block->n);

    pthread_mutex_lock(&g_lock);
    g_blocks.erase(block->real);
    pthread_mutex_unlock(&g_lock);
    malloc_impl_free(block);
}

static void* malloc_impl_realloc_debug(void *p, UInt32 n) {
    FATAL_CHECK_GT(n, 0);
    MallocBlock *old = validateBlock(p);
    if (old == Nil) {
        INFO("realloc unregisterred pointer %p", p);
        return malloc_impl_realloc(p, n);
    }
    // always erase old entry
    // as new block and old block may have overlap
    pthread_mutex_lock(&g_lock);
    g_blocks.erase(old->real);
    pthread_mutex_unlock(&g_lock);

    const UInt32 length = sizeof(MallocBlock) + n + sizeof(UInt32);
    MallocBlock * block = (MallocBlock*)malloc_impl_realloc(old, length);
    block->magic        = 0xbaaddead;
    block->n            = n;
    block->flags        = 0;
    block->real         = &block[1];
    block->stack_size   = CallStackGet(block->stack, 16);
    ((UInt32*)((Char*)block + length))[-1] = 0xdeadbaad;

    CHECK_NULL(block);
    DEBUG("realloc %p@%p -> %p%p", old->real, old, block->real, block);

    pthread_mutex_lock(&g_lock);
    g_blocks.insert(block->real, block);
    pthread_mutex_unlock(&g_lock);
    return block->real;
}

static Int malloc_impl_posix_memalign_debug(void **memptr, UInt32 alignment, UInt32 n) {
    FATAL_CHECK_GT(n, 0);
    FATAL_CHECK_GT(alignment, 0);
    FATAL_CHECK_EQ(sizeof(MallocBlock) % alignment, 0);   // FIXME
    const UInt32 length = sizeof(MallocBlock) + n + sizeof(UInt32);
    MallocBlock * block;
    Int ret = malloc_impl_posix_memalign((void**)&block, length, alignment);
    if (ret < 0) {
        return ret;
    }
    block->magic        = 0xbaaddead;
    block->n            = n;
    block->flags        = 0;
    block->real         = &block[1];
    block->stack_size   = CallStackGet(block->stack, 16);
    ((UInt32*)((Char*)block + length))[-1] = 0xdeadbaad;
    CHECK_NULL(block);

    pthread_mutex_lock(&g_lock);
    g_blocks.insert(block->real, block);
    pthread_mutex_unlock(&g_lock);
    *memptr = block->real;
    return 0;
}

//////////////////////////////////////////////////////////////////////////////
static real_malloc_t g_malloc_impl_malloc = malloc_impl_malloc;
static real_free_t g_malloc_impl_free = malloc_impl_free;
static real_realloc_t g_malloc_impl_realloc = malloc_impl_realloc;
static real_posix_memalign_t g_malloc_impl_posix_memalign = malloc_impl_posix_memalign;

ABE_EXPORT_C void * malloc(size_t n) {
    DEBUG("malloc %zu", n);
    //BTRACE();
    return g_malloc_impl_malloc(n);
}

ABE_EXPORT_C void* calloc(size_t count, size_t n) {
    void *p = malloc(count * n);
    CHECK_NULL(p);
    memset(p, 0, count * n);
    return p;
}

ABE_EXPORT_C void free(void *p) {
    g_malloc_impl_free(p);
}

ABE_EXPORT_C void* realloc(void *p, size_t n) {
    return g_malloc_impl_realloc(p, n);
}

ABE_EXPORT_C Int posix_memalign(void **memptr, size_t alignment, size_t n) {
    return g_malloc_impl_posix_memalign(memptr, alignment, n);
}

//////////////////////////////////////////////////////////////////////////////
ABE_EXPORT_C void MemoryAnalyzerPrepare() {
    g_malloc_impl_malloc            = malloc_impl_malloc_debug;
    g_malloc_impl_free              = malloc_impl_free_debug;
    g_malloc_impl_realloc           = malloc_impl_realloc_debug;
    g_malloc_impl_posix_memalign    = malloc_impl_posix_memalign_debug;
}

ABE_EXPORT_C void MemoryAnalyzerBypass() {
    g_malloc_impl_malloc            = malloc_impl_malloc;
    g_malloc_impl_free              = malloc_impl_free;
    g_malloc_impl_realloc           = malloc_impl_realloc;
    g_malloc_impl_posix_memalign    = malloc_impl_posix_memalign;
}

ABE_EXPORT_C void MemoryAnalyzerFinalize() {
    printBlocks();
    MemoryAnalyzerBypass();
}

__END_NAMESPACE_ABE

ABE_EXPORT_C Char* strndup(const Char *s, size_t n) {
    Char *p = (Char *)malloc(n + 1);
    strncpy(p, s, n);
    ((Char*)(p))[n] = '\0';
    return (Char*)p;
}

ABE_EXPORT_C Char* strdup(const Char *s) {
    Char *p = strndup(s, strlen(s));
    return p;
}

ABE_EXPORT void * operator new(size_t n) {
    return __NAMESPACE_ABE::g_malloc_impl_malloc(n);
}

ABE_EXPORT void * operator new[](size_t n) {
    return __NAMESPACE_ABE::g_malloc_impl_malloc(n);
}

ABE_EXPORT void operator delete(void* ptr) {
    return __NAMESPACE_ABE::g_malloc_impl_free(ptr);
}

ABE_EXPORT void operator delete[](void* ptr) {
    return __NAMESPACE_ABE::g_malloc_impl_free(ptr);
}

ABE_EXPORT_C void __cxa_pure_virtual() {
    ERROR("access object after release");
    UInt64 stack[32];
    UInt32 n = CallStackGet(stack, 32);
    CallStackPut(stack, n);
}