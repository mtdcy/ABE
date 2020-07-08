#define LOG_TAG "test_c"
#include <ABE/ABE.h>

#include <string.h>

extern void MemoryAnalyzerPrepare();
extern void MemoryAnalyzerBypass();
extern void MemoryAnalyzerFinalize();

void testBuffer() {
    MemoryAnalyzerPrepare();
    
    // create a shared buffer
    SharedBufferRef shared = SharedBufferCreate(AllocatorGetDefault(), 1024);
    CHECK_EQ(SharedObjectGetRetainCount(shared), 1);
    
    SharedBufferRef copy = SharedObjectRetain(shared);
    CHECK_EQ(SharedObjectGetRetainCount(copy), 2);
    SharedBufferRelease(copy);
    
    CHECK_EQ(SharedObjectGetRetainCount(shared), 1);
    SharedBufferRelease(shared);
    
    MemoryAnalyzerFinalize();
}

int main (int argc, Char ** argv) {
    
    testBuffer();
    
    return 0;
}
