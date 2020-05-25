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

//#define DEBUG_MALLOC
#ifdef DEBUG_MALLOC
#define _DARWIN_C_SOURCE    // for RTLD_NEXT
//#define _GNU_SOURCE         // for RTLD_NEXT on Linux

#define LOG_TAG  "debug.malloc"
//#define LOG_NDEBUG 0
#include "core/Log.h"

#include "core/Types.h"

#include "backtrace.h"
#define MALLOC_CALLSTACK
#define MALLOC_DEFAULT_BYPASS

#include <string.h>
#include <pthread.h>
#include <dlfcn.h>

#include "stl/HashTable.h"

// we can not use CHECK_EQ/... here, which use String
#define FATAL_CHECK(a, b, op) do {                                  \
    if (__builtin_expect(!((a) op (b)), 0)) {                       \
        FATAL(__FILE__ ":" LITERAL_TO_STRING(__LINE__)              \
                " CHECK(" #a " " #op " " #b ") failed. ");          \
    }                                                               \
} while(0)

#define FATAL_CHECK_EQ(a, b)    FATAL_CHECK(a, b, ==)
#define FATAL_CHECK_GE(a, b)    FATAL_CHECK(a, b, >=)
#define FATAL_CHECK_GT(a, b)    FATAL_CHECK(a, b, >)

USING_NAMESPACE_ABE

#ifdef __GLIBC__
#define USING_DLSYM     0
#else
#define USING_DLSYM     1
#endif

typedef void*   (*real_malloc_t)(size_t n);
typedef void    (*real_free_t)(void *p);
typedef void*   (*real_realloc_t)(void *p, size_t n);
typedef int     (*real_posix_memalign_t)(void **, size_t, size_t);
#if USING_DLSYM == 0
// http://stackoverflow.com/questions/5223971/question-about-overriding-c-standard-library-functions-and-how-to-link-everythin
//extern void *__libc_malloc(size_t);
//extern void  __libc_free(void *);
//extern void *__libc_realloc(void *, size_t);
extern __typeof (malloc) __libc_malloc;
extern __typeof (free) __libc_free;
extern __typeof (realloc) __libc_realloc;
#endif

// can NOT garentee static global variable initialization order,
// so put it inside function as static local variable.
static ABE_INLINE void * real_malloc(size_t size) {
#if USING_DLSYM == 0
    static real_malloc_t _real_malloc = __libc_malloc;
#else
    static real_malloc_t _real_malloc = NULL;
#endif
    if (__builtin_expect(_real_malloc == NULL, false)) {
        DEBUG("initial real_malloc");
        _real_malloc = (real_malloc_t)dlsym(RTLD_NEXT, "malloc");
    }
    return _real_malloc(size);
}

static ABE_INLINE void real_free(void *ptr) {
#if USING_DLSYM == 0
    static real_free_t _real_free = __libc_free;
#else
    static real_free_t _real_free = NULL;
#endif
    if (__builtin_expect(_real_free == NULL, false)) {
        DEBUG("initial real_free");
        _real_free = (real_free_t)dlsym(RTLD_NEXT, "free");
    }
    return _real_free(ptr);
}

static ABE_INLINE void * real_realloc(void *ptr, size_t size) {
#if USING_DLSYM == 0
    static real_realloc_t _real_realloc = __libc_realloc;
#else
    static real_realloc_t _real_realloc = NULL;
#endif
    if (__builtin_expect(_real_realloc == NULL, false)) {
        DEBUG("initial real_realloc");
        _real_realloc = (real_realloc_t)dlsym(RTLD_NEXT, "realloc");
    }
    return _real_realloc(ptr, size);
}

static ABE_INLINE int real_posix_memalign(void **memptr, size_t alignment, size_t n) {
    static real_posix_memalign_t _real_posix_memalign = NULL;
    if (__builtin_expect(_real_posix_memalign == NULL, false)) {
        DEBUG("initial real_posix_memalign");
        _real_posix_memalign = (real_posix_memalign_t)dlsym(RTLD_NEXT, "posix_memalign");
    }
    return _real_posix_memalign(memptr, alignment, n);
}

struct RealAllocator : public Allocator {
    virtual void *  allocate(size_t size) { return real_malloc(size); }
    virtual void *  reallocate(void * ptr, size_t size) { return real_realloc(ptr, size); }
    virtual void    deallocate(void * ptr) { return real_free(ptr); }
};

struct MallocBlock {
    uint32_t        magic;
    size_t          n;
    uint32_t        flags;
#ifdef MALLOC_CALLSTACK
    uint32_t        stack_size;
    bt_stack_t      stack[32];
#else
    uint32_t        dummy1;
#endif
    void *          real;
    void *          dummy2;
};

#if 0
// find a better hash function ???
// 1. The address of a block returned by malloc or realloc in the GNU system is
//    always a multiple of eight (or sixteen on 64-bit systems)
// 2. https://www.cocoawithlove.com/2010/05/look-at-how-malloc-works-on-mac.html
//
static ABE_INLINE size_t do_hash(void * p) {
#if __LP64__
    return (size_t)((((intptr_t)p >> 32) | (intptr_t)p) / 16);
#else
    return (size_t)((((intptr_t)p >> 32) | (intptr_t)p) / 8);
#endif
}
#endif

static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;
static HashTable<void *, MallocBlock *> g_blocks(8192, new RealAllocator);

static ABE_INLINE void printBlocks() {
    size_t n = 0;
    size_t total = 0;
    HashTable<void *, MallocBlock*>::const_iterator it = g_blocks.cbegin();
    INFO("===============================================================");
    INFO("== current malloc info: ");
    for (; it != g_blocks.cend(); ++it) {
        const MallocBlock* block = it.value();
        INFO("== %p - %" PRIu32, block->real, block->n);
#ifdef MALLOC_CALLSTACK
        backtrace_symbols(block->stack, block->stack_size);
#endif
        ++n;
        total += block->n;
    }
    INFO("== total %zu bytes(%zu)", total, n);
    INFO("===============================================================");
}

static ABE_INLINE MallocBlock* validateBlock(void *p) {
    CHECK_NULL(p);

    pthread_mutex_lock(&g_lock);
    MallocBlock * block = *g_blocks.find(p);
    pthread_mutex_unlock(&g_lock);

    if (block == NULL) return NULL;

    const size_t length = sizeof(struct MallocBlock) + block->n + sizeof(uint32_t);
    FATAL_CHECK_EQ(block->magic, 0xbaaddead);
    uint32_t magic = ((uint32_t*)((char*)block + length))[-1];
    FATAL_CHECK_EQ(magic, 0xdeadbaad);
    return block;
}

//////////////////////////////////////////////////////////////////////////////
static void* malloc_impl_malloc_bypass(size_t n);
static void* malloc_impl_malloc_body(size_t n);
static void malloc_impl_free_bypass(void *p);
static void  malloc_impl_free_body(void *p);
static void* malloc_impl_realloc_bypass(void *p, size_t n);
static void* malloc_impl_realloc_body(void *p, size_t n);
static int malloc_impl_posix_memalign_bypass(void **memptr, size_t alignment, size_t n);
static int malloc_impl_posix_memalign_body(void **memptr, size_t alignment, size_t n);

// default: always bypass, wait intialization finished
static real_malloc_t g_malloc_impl_malloc = malloc_impl_malloc_bypass;
static real_free_t g_malloc_impl_free = malloc_impl_free_bypass;
static real_realloc_t g_malloc_impl_realloc = malloc_impl_realloc_bypass;
static real_posix_memalign_t g_malloc_impl_posix_memalign = malloc_impl_posix_memalign_bypass;

static void* malloc_impl_malloc_bypass(size_t n) {
    DEBUG("malloc %zu", n);
    return real_malloc(n);
}

static void* malloc_impl_malloc_body(size_t n) {
    FATAL_CHECK_GT(n, 0);
    size_t length = sizeof(MallocBlock) + n + sizeof(uint32_t);
    MallocBlock * block = (MallocBlock*)real_malloc(length);
    block->magic        = 0xbaaddead;
    block->n            = n;
    block->flags        = 0;
    block->real         = &block[1];
#ifdef MALLOC_CALLSTACK
    block->stack_size   = backtrace_stack(block->stack, 32);
#endif
    ((uint32_t*)((char*)block + length))[-1] = 0xdeadbaad;

    DEBUG("malloc %p@%p[%zu]", block->real, block, block->n);

    CHECK_NULL(block);
    pthread_mutex_lock(&g_lock);
    g_blocks.insert(block->real, block);
    pthread_mutex_unlock(&g_lock);
    return block->real;
}

static void malloc_impl_free_bypass(void *p) {
    return real_free(p);
}

static void  malloc_impl_free_body(void *p) {
    MallocBlock *block = validateBlock(p);

    if (block == NULL) {
        // this should NOT happen if others' code are written in a proper way,
        // but no one can garentee that, like backtrace_symbols
        // XXX: malloc a block inside library, and ask someone else to free it 
        // is not a good program manner, even simply using free().
#if LOG_NDEBUG == 0
        DEBUG("free unregisterred pointer %p", p);
#ifdef MALLOC_CALLSTACK
        BACKTRACE();
#endif
#endif
        return real_free(p);
    }
    DEBUG("free %p@%p[%zu]", block->real, block, block->n);

    pthread_mutex_lock(&g_lock);
    g_blocks.erase(block->real);
    pthread_mutex_unlock(&g_lock);
    real_free(block);
}

static void* malloc_impl_realloc_bypass(void *p, size_t n) {
    return real_realloc(p, n);
}

static void* malloc_impl_realloc_body(void *p, size_t n) {
    FATAL_CHECK_GT(n, 0);
    MallocBlock *old = validateBlock(p);
    if (old == NULL) {
        INFO("realloc unregisterred pointer %p", p);
        return real_realloc(p, n);
    }
    // always erase old entry
    // as new block and old block may have overlap
    pthread_mutex_lock(&g_lock);
    g_blocks.erase(old->real);
    pthread_mutex_unlock(&g_lock);

    const size_t length = sizeof(MallocBlock) + n + sizeof(uint32_t);
    MallocBlock * block = (MallocBlock*)real_realloc(old, length);
    block->magic        = 0xbaaddead;
    block->n            = n;
    block->flags        = 0;
    block->real         = &block[1];
#ifdef MALLOC_CALLSTACK
    block->stack_size   = backtrace_stack(block->stack, 32);
#endif
    ((uint32_t*)((char*)block + length))[-1] = 0xdeadbaad;

    CHECK_NULL(block);
    DEBUG("realloc %p@%p -> %p%p", old->real, old, block->real, block);

    pthread_mutex_lock(&g_lock);
    g_blocks.insert(block->real, block);
    pthread_mutex_unlock(&g_lock);
    return block->real;
}

static int malloc_impl_posix_memalign_bypass(void **memptr, size_t alignment, size_t n) {
    return real_posix_memalign(memptr, alignment, n);
}

static int malloc_impl_posix_memalign_body(void **memptr, size_t alignment, size_t n) {
    FATAL_CHECK_GT(n, 0);
    FATAL_CHECK_GT(alignment, 0);
    FATAL_CHECK_EQ(sizeof(MallocBlock) % alignment, 0);   // FIXME
    const size_t length = sizeof(MallocBlock) + n + sizeof(uint32_t);
    MallocBlock * block;
    int ret = real_posix_memalign((void**)&block, length, alignment);
    if (ret < 0) {
        return ret;
    }
    block->magic        = 0xbaaddead;
    block->n            = n;
    block->flags        = 0;
    block->real         = &block[1];
#ifdef MALLOC_CALLSTACK
    block->stack_size   = backtrace_stack(block->stack, 32);
#endif
    ((uint32_t*)((char*)block + length))[-1] = 0xdeadbaad;
    CHECK_NULL(block);

    pthread_mutex_lock(&g_lock);
    g_blocks.insert(block->real, block);
    pthread_mutex_unlock(&g_lock);
    *memptr = block->real;
    return 0;
}

//////////////////////////////////////////////////////////////////////////////
__BEGIN_DECLS

ABE_EXPORT void* malloc(size_t n) {
    DEBUG("malloc %zu", n);
    //BTRACE();
    return g_malloc_impl_malloc(n);
}

ABE_EXPORT void* calloc(size_t count, size_t n) {
    void *p = malloc(count * n);
    CHECK_NULL(p);
    memset(p, 0, count * n);
    return p;
}

ABE_EXPORT void free(void *p) {
    g_malloc_impl_free(p);
}

ABE_EXPORT void* realloc(void *p, size_t n) {
    return g_malloc_impl_realloc(p, n);
}

ABE_EXPORT int posix_memalign(void **memptr, size_t alignment, size_t n) {
    return g_malloc_impl_posix_memalign(memptr, alignment, n);
}

#if 1
ABE_EXPORT char* strndup(const char *s, size_t n) {
    char *p = (char *)malloc(n + 1);
    strncpy(p, s, n);
    ((char*)(p))[n] = '\0';
    return (char*)p;
}

ABE_EXPORT char* strdup(const char *s) {
    char *p = strndup(s, strlen(s));
    return p;
}
#endif

#if 0
void * operator new(size_t n) throw(std::bad_alloc) {
    return g_malloc_impl_malloc(n);
}

void * operator new[](size_t n) throw(std::bad_alloc) {
    return g_malloc_impl_malloc(n);
}

void operator delete(void* ptr) throw() {
    return g_malloc_impl_free(ptr);
}

void operator delete[](void* ptr) throw() {
    return g_malloc_impl_free(ptr);
}
#endif

//////////////////////////////////////////////////////////////////////////////
ABE_EXPORT void malloc_prepare() {
    g_malloc_impl_malloc            = malloc_impl_malloc_body;
    g_malloc_impl_free              = malloc_impl_free_body;
    g_malloc_impl_realloc           = malloc_impl_realloc_body;
    g_malloc_impl_posix_memalign    = malloc_impl_posix_memalign_body;
}

ABE_EXPORT void malloc_bypass() {
    g_malloc_impl_malloc            = malloc_impl_malloc_bypass;
    g_malloc_impl_free              = malloc_impl_free_bypass;
    g_malloc_impl_realloc           = malloc_impl_realloc_bypass;
    g_malloc_impl_posix_memalign    = malloc_impl_posix_memalign_bypass;
}

ABE_EXPORT void malloc_finalize() {
    if (g_blocks.size()) printBlocks();
    malloc_bypass();
}

__END_DECLS

#else

#include "core/Types.h"
__BEGIN_DECLS
ABE_EXPORT void malloc_prepare() { }
ABE_EXPORT void malloc_bypass() { }
ABE_EXPORT void malloc_finalize() { }
__END_DECLS

#endif

