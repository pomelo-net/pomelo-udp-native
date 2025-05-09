#include "pomelo-test.h"
#include "utils-test.h"
#include "utils/array.h"


int pomelo_test_array(void) {
    pomelo_allocator_t * allocator = pomelo_allocator_default();
    uint64_t alloc_bytes = pomelo_allocator_allocated_bytes(allocator);

    pomelo_array_options_t options = {
        .allocator = allocator,
        .element_size = sizeof(int),
    };
    pomelo_array_t * array = pomelo_array_create(&options);
    pomelo_check(array != NULL);
    pomelo_check(pomelo_allocator_allocated_bytes(allocator) > alloc_bytes);

    int i, value;
    // Set the data
    for (i = 0; i < 1000; ++i) {
        value = 2 * i;
        pomelo_check(pomelo_array_append_ptr(array, &value) != NULL);
    }

    int * output;
    // Get the data
    for (i = 0; i < 1000; ++i) {
        output = pomelo_array_get_ptr(array, i);
        pomelo_check(output != NULL);
        pomelo_check(*output == 2 * i);
    }

    pomelo_check(pomelo_array_resize(array, 10) == 0);
    pomelo_check(array->size == 10);

    // Destroy the array
    pomelo_array_destroy(array);

    // Check memleak
    pomelo_check(alloc_bytes == pomelo_allocator_allocated_bytes(allocator));
    return 0;
}
