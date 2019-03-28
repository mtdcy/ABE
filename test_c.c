#define LOG_TAG "test_c"
#include <ABE/ABE.h>

#include <string.h>

extern void malloc_prepare();
extern void malloc_bypass();
extern void malloc_finalize();

void testBuffer() {
    malloc_prepare();
    
    // create a shared buffer
    SharedBuffer * shared = SharedBufferCreate(AllocatorGetDefault(), 1024);
    CHECK_EQ(SharedBufferGetRetainCount(shared), 1);
    
    SharedBuffer * copy = SharedBufferRetain(shared);
    CHECK_EQ(SharedBufferGetRetainCount(copy), 2);
    SharedBufferRelease(copy);
    
    CHECK_EQ(SharedBufferGetRetainCount(shared), 1);
    SharedBufferRelease(shared);
    
    malloc_finalize();
}

int main (int argc, char ** argv) {
    
    testBuffer();
    
    return 0;
}
