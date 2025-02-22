#include "pomelo-test.h"
#include "utils/pool.h"
#include "base-test.h"


static int finalized = 0;
static int alloc_counter = 0;

static int pomelo_pool_test_init(int * element, void * context) {
    (void) context;
    *element = 1;
    alloc_counter++;
    return 0;
}

static int pomelo_pool_test_acquire(int * element, void * context) {
    (void) context;
    *element = *element + 1;  // 2
    return 0;
}

static int pomelo_pool_test_release(int * element, void * context) {
    (void) context;
    *element = *element * 2;  // 4
    return 0;
}

static int pomelo_pool_test_finalize(int * element, void * context) {
    (void) element;
    (void) context;
    finalized = 1;
    alloc_counter--;
    return 0;
}

int pomelo_test_pool(void) {
    pomelo_allocator_t * allocator = pomelo_allocator_default();
    // Save the current bytes
    uint64_t alloc_bytes = pomelo_allocator_allocated_bytes(allocator);

    // Create pools
    pomelo_pool_options_t options;
    pomelo_pool_options_init(&options);
    options.allocator = allocator;
    options.element_size = sizeof(int);
    options.callback_context = NULL;
    options.allocate_callback = (pomelo_pool_callback)
        pomelo_pool_test_init;
    options.acquire_callback = (pomelo_pool_callback)
        pomelo_pool_test_acquire;
    options.release_callback = (pomelo_pool_callback)
        pomelo_pool_test_release;
    options.deallocate_callback = (pomelo_pool_callback)
        pomelo_pool_test_finalize;

    pomelo_pool_t * pool = pomelo_pool_create(&options);
    pomelo_check(pool != NULL);


    pomelo_check(pool->available_elements == NULL);
    pomelo_check(pool->allocated_elements == NULL);

    // Create shared pool
    pomelo_shared_pool_options_t shared_options;
    pomelo_shared_pool_options_init(&shared_options);
    shared_options.allocator = allocator;
    shared_options.buffers = 2;
    shared_options.master_pool = pool;

    pomelo_shared_pool_t * shared_pool =
        pomelo_shared_pool_create(&shared_options);
    pomelo_check(shared_pool != NULL);

    // Get some elements from pools
    int * data = pomelo_pool_acquire(pool);
    pomelo_check(data != NULL);
    pomelo_check(*data == 2);

    pomelo_check(pool->available_elements == NULL);
    pomelo_check(pool->allocated_elements != NULL);

    int * data2 = pomelo_pool_acquire(pool);
    pomelo_check(data2 != NULL);
    pomelo_check(*data == 2);

    pomelo_pool_release(pool, data);
    pomelo_check(pool->available_elements != NULL);
    pomelo_check(pool->allocated_elements != NULL);

    data = pomelo_pool_acquire(pool);

    pomelo_check(pool->available_elements == NULL);
    pomelo_check(pool->allocated_elements != NULL);

    size_t in_use = pomelo_pool_in_use(pool);

    int * array[10];
    // int acquired = pomelo_pool_acquire_batch(pool, (void **) array, 10);
    // pomelo_check(acquired == 10);
    // pomelo_check(pool->in_use_elements == (in_use + 10));

    // pomelo_pool_release_batch(pool, (void **) array, 10);
    // pomelo_check(pool->in_use_elements == in_use);

    // Test shared pool
    for (int i = 0; i < 5; i++) {
        array[i] = pomelo_shared_pool_acquire(shared_pool);
    }

    // Inuse = 6
    pomelo_check(pomelo_pool_in_use(pool) == (in_use + 6));

    // Inuse = 4
    for (int i = 0; i < 5; i++) {
        pomelo_shared_pool_release(shared_pool, array[i]);
    }

    pomelo_check(pomelo_pool_in_use(pool) == (in_use + 4));

    void * last_element = pomelo_shared_pool_acquire(shared_pool);
    pomelo_shared_pool_release(shared_pool, last_element);

    // Destroy shared pool
    pomelo_shared_pool_destroy(shared_pool);

    // Check if all elements have been released
    pomelo_check(pomelo_pool_in_use(pool) == in_use);

    // Destroy pools
    pomelo_pool_destroy(pool);

    pomelo_check(finalized == 1);
    pomelo_check(alloc_counter == 0);

    // Check for memleak
    pomelo_check(pomelo_allocator_allocated_bytes(allocator) == alloc_bytes);

    return 0;
}
