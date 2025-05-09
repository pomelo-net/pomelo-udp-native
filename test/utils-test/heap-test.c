#include "pomelo-test.h"
#include "utils/heap.h"
#include "utils-test.h"


static int pomelo_compare_int(void * a, void * b) {
    pomelo_check(a != NULL);
    pomelo_check(b != NULL);

    // return *(int *)b - *(int *)a;
    return *(int *)a - *(int *)b;
}


int pomelo_test_heap(void) {
    pomelo_allocator_t * allocator = pomelo_allocator_default();
    
    uint64_t alloc_bytes = pomelo_allocator_allocated_bytes(allocator);
    pomelo_heap_options_t options = {
        .element_size = sizeof(int),
        .allocator = allocator,
        .compare = (pomelo_heap_compare_fn) pomelo_compare_int,
    };

    // Create the heap
    pomelo_heap_t * heap = pomelo_heap_create(&options);
    pomelo_check(heap != NULL);
    pomelo_check(alloc_bytes < pomelo_allocator_allocated_bytes(allocator));

    pomelo_check(pomelo_heap_size(heap) == 0);

    // Test inserting elements
    int value = 5;
    pomelo_check(pomelo_heap_push(heap, value) != NULL);
    pomelo_check(pomelo_heap_size(heap) == 1);

    value = 3;
    pomelo_check(pomelo_heap_push(heap, value) != NULL);
    pomelo_check(pomelo_heap_size(heap) == 2);

    value = 7;
    pomelo_check(pomelo_heap_push(heap, value) != NULL);
    pomelo_check(pomelo_heap_size(heap) == 3);

    value = 2;
    pomelo_check(pomelo_heap_push(heap, value) != NULL);
    pomelo_check(pomelo_heap_size(heap) == 4);

    value = 9;
    pomelo_check(pomelo_heap_push(heap, value) != NULL);
    pomelo_check(pomelo_heap_size(heap) == 5);

    value = 4;
    pomelo_check(pomelo_heap_push(heap, value) != NULL);
    pomelo_check(pomelo_heap_size(heap) == 6);

    value = 1;
    pomelo_heap_entry_t * entry = pomelo_heap_push(heap, value);
    pomelo_check(entry != NULL);
    pomelo_check(pomelo_heap_size(heap) == 7);

    // Value 1 is removed
    pomelo_heap_remove(heap, entry);
    pomelo_check(pomelo_heap_size(heap) == 6);

    // Test popping elements in order
    pomelo_check(pomelo_heap_top(heap, &value) == 0);
    pomelo_check(value == 2);
    pomelo_check(pomelo_heap_pop(heap, &value) == 0);
    pomelo_check(value == 2);
    pomelo_check(pomelo_heap_size(heap) == 5);

    pomelo_check(pomelo_heap_top(heap, &value) == 0);
    pomelo_check(value == 3);
    pomelo_check(pomelo_heap_pop(heap, &value) == 0);
    pomelo_check(value == 3);
    pomelo_check(pomelo_heap_size(heap) == 4);

    pomelo_check(pomelo_heap_top(heap, &value) == 0);
    pomelo_check(value == 4);
    pomelo_check(pomelo_heap_pop(heap, &value) == 0);
    pomelo_check(value == 4);
    pomelo_check(pomelo_heap_size(heap) == 3);

    pomelo_check(pomelo_heap_top(heap, &value) == 0);
    pomelo_check(value == 5);
    pomelo_check(pomelo_heap_pop(heap, &value) == 0);
    pomelo_check(value == 5);
    pomelo_check(pomelo_heap_size(heap) == 2);

    pomelo_check(pomelo_heap_top(heap, &value) == 0);
    pomelo_check(value == 7);
    pomelo_check(pomelo_heap_pop(heap, &value) == 0);
    pomelo_check(value == 7);
    pomelo_check(pomelo_heap_size(heap) == 1);

    pomelo_check(pomelo_heap_top(heap, &value) == 0);
    pomelo_check(value == 9);
    pomelo_check(pomelo_heap_pop(heap, &value) == 0);
    pomelo_check(value == 9);
    pomelo_check(pomelo_heap_size(heap) == 0);

    // Test empty heap
    pomelo_check(pomelo_heap_size(heap) == 0);
    pomelo_check(pomelo_heap_top(heap, &value) == -1);
    pomelo_check(pomelo_heap_pop(heap, &value) == -1);

    pomelo_heap_destroy(heap);
    return 0;
}
