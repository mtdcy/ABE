#define LOG_TAG "test_c"
#include <ABE/ABE.h>

#include <string.h>

extern void malloc_prepare();
extern void malloc_bypass();
extern void malloc_finalize();

void testBuffer() {
    malloc_prepare();
    
    // create a shared buffer
    SharedBufferRef shared = SharedBufferCreate(AllocatorGetDefault(), 1024);
    CHECK_EQ(SharedObjectGetRetainCount(shared), 1);
    
    SharedBufferRef copy = SharedObjectRetain(shared);
    CHECK_EQ(SharedObjectGetRetainCount(copy), 2);
    SharedBufferRelease(copy);
    
    CHECK_EQ(SharedObjectGetRetainCount(shared), 1);
    SharedBufferRelease(shared);
    
    malloc_finalize();
}

int main (int argc, char ** argv) {
    
    testBuffer();
    
    return 0;
}
