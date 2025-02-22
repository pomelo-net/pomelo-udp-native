#include "utils/list.h"
#include "pomelo-test.h"
#include "base-test.h"


int pomelo_test_unrolled_list(void) {
    pomelo_allocator_t * allocator = pomelo_allocator_default();
    uint64_t alloc_bytes = pomelo_allocator_allocated_bytes(allocator);

    pomelo_unrolled_list_options_t options;
    options.allocator = allocator;
    options.element_size = sizeof(int);
    options.bucket_elements = 16;
    int ret, value;

    // Create the list
    pomelo_unrolled_list_t * list = pomelo_unrolled_list_create(&options);
    pomelo_check(list != NULL);
    
    pomelo_check(list->size == 0);

    ret = pomelo_unrolled_list_pop_back(list, &value);
    pomelo_check(ret != 0);

    // Test push back API
    for (size_t i = 0; i < 33; i++) {
        ret = pomelo_unrolled_list_push_back(list, i);
        pomelo_check(ret == 0);
        pomelo_check(list->size == (i + 1));
    }

    pomelo_check(list->nodes->size == 3);

    // Test set/get API
    ret = pomelo_unrolled_list_get(list, 9, &value);
    pomelo_check(ret == 0);
    pomelo_check(value == 9);

    value = 90;
    ret = pomelo_unrolled_list_set(list, 10, value);
    pomelo_check(ret == 0);

    ret = pomelo_unrolled_list_set(list, -12, value);
    pomelo_check(ret != 0);

    ret = pomelo_unrolled_list_set(list, 33, value);
    pomelo_check(ret != 0);

    // Test pop back API
    ret = pomelo_unrolled_list_pop_back(list, &value);
    pomelo_check(ret == 0);

    pomelo_check(value == 32);
    pomelo_check(list->size == 32);

    pomelo_check(list->nodes->size == 2);

    // Test iterator
    pomelo_unrolled_list_iterator_t it;
    pomelo_unrolled_list_begin(list, &it);

    size_t index = 0;
    while (pomelo_unrolled_list_iterator_next(&it, &value)) {
        if (index == 10) {
            pomelo_check(value == 90);
        } else {
            pomelo_check(value == (int) index);
        }
        index++;
    }

    pomelo_unrolled_list_end(list, &it);
    index = list->size - 1;
    while (pomelo_unrolled_list_iterator_prev(&it, &value)) {
        if (index == 10) {
            pomelo_check(value == 90);
        } else {
            pomelo_check(value == (int) index);
        }
        index--;
    }

    pomelo_unrolled_list_destroy(list);

    pomelo_check(alloc_bytes == pomelo_allocator_allocated_bytes(allocator));
    return 0;
}
