#include "pomelo/allocator.h"
#include "pomelo-test.h"
#include "base-test.h"


int pomelo_test_allocator(void) {
    // Test default allocator
    pomelo_allocator_t * allocator = pomelo_allocator_default();

    pomelo_check(allocator != NULL);
    void * mem = pomelo_allocator_malloc(allocator, 128);
    pomelo_check(pomelo_allocator_allocated_bytes(allocator) == 128);
    pomelo_check(mem != NULL);
    pomelo_allocator_free(allocator, mem);
    pomelo_check(pomelo_allocator_allocated_bytes(allocator) == 0);

    return 0;
}
