#include "pomelo-test.h"
#include "utils/list.h"
#include "base-test.h"


int pomelo_test_list(void) {
    pomelo_allocator_t * allocator = pomelo_allocator_default();
    pomelo_list_options_t options;
    
    uint64_t alloc_bytes = pomelo_allocator_allocated_bytes(allocator);

    pomelo_list_options_init(&options);
    options.element_size = sizeof(int);
    options.allocator = allocator;
    options.synchronized = true;

    // Create the list
    pomelo_list_t * list = pomelo_list_create(&options);
    pomelo_check(list != NULL);
    pomelo_check(alloc_bytes < pomelo_allocator_allocated_bytes(allocator));

    pomelo_check(list->size == 0);

    int value = 1;
    pomelo_check(pomelo_list_push_back(list, value) != NULL);
    pomelo_check(list->size == 1);

    value = 2;
    pomelo_list_node_t * second = pomelo_list_push_back(list, value);
    pomelo_check(second != NULL);
    pomelo_check(list->size == 2);

    value = 3;
    pomelo_check(pomelo_list_push_back(list, value) != NULL);
    pomelo_check(list->size == 3);

    pomelo_list_remove(list, second);
    pomelo_check(list->size == 2);

    value = 4;
    pomelo_check(pomelo_list_push_back(list, value) != NULL);
    pomelo_check(list->size == 3);

    value = 5;
    pomelo_check(pomelo_list_push_front(list, value) != NULL);
    pomelo_check(list->size == 4);

    printf("List values: ");
    pomelo_list_for(list, value, int, {
        printf("%d, ", value);
    });
    printf("\n");

    // The list data: [5, 1, 3, 4]
    pomelo_check(pomelo_list_pop_front(list, &value) == 0);
    pomelo_check(value == 5);
    pomelo_check(pomelo_list_pop_front(list, &value) == 0);
    pomelo_check(value == 1);
    pomelo_check(pomelo_list_pop_back(list, &value) == 0);
    pomelo_check(value == 4);
    pomelo_check(pomelo_list_pop_back(list, &value) == 0);
    pomelo_check(value == 3);
    pomelo_check(pomelo_list_pop_back(list, &value) != 0);
    pomelo_check(pomelo_list_pop_front(list, &value) != 0);

    pomelo_check(list->front == NULL);
    pomelo_check(list->back == NULL);
    pomelo_check(list->size == 0);

    pomelo_list_destroy(list);


    // Check memleak
    pomelo_check(alloc_bytes == pomelo_allocator_allocated_bytes(allocator));
    return 0;
}
