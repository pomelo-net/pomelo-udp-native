#include "pomelo-test.h"
#include "utils/map.h"
#include "utils-test.h"


int pomelo_test_map(void) {
    pomelo_allocator_t * allocator = pomelo_allocator_default();
    uint64_t alloc_bytes = pomelo_allocator_allocated_bytes(allocator);

    pomelo_map_options_t options = {
        .allocator = allocator,
        .key_size = sizeof(int),
        .value_size = sizeof(int),
    };
    pomelo_map_t * map = pomelo_map_create(&options);
    pomelo_check(map != NULL);

    int key = 1;
    int value = 1000;

    pomelo_check(pomelo_map_set(map, key, value) != NULL);

    int val_read = 0;
    pomelo_check(pomelo_map_get(map, key, &val_read) == 0);
    pomelo_check(val_read == value);
    pomelo_check(map->size == 1);

    // Update the existent key
    value = 2000;
    pomelo_check(pomelo_map_set(map, key, value) != NULL);
    pomelo_check(map->size == 1); // Unchanged
    pomelo_check(pomelo_map_get(map, key, &val_read) == 0);
    pomelo_check(val_read == value);

    key = 2;
    // Get non-existent key
    pomelo_check(pomelo_map_get(map, key, &val_read) < 0);

    value = 3000;
    pomelo_check(pomelo_map_set(map, key, value) != NULL);
    pomelo_check(pomelo_map_has(map, key));
    pomelo_check(map->size == 2);

    // Delete existent key
    pomelo_check(pomelo_map_del(map, key) == 0);
    pomelo_check(map->size == 1);

    pomelo_check(pomelo_map_del(map, key) < 0);
    pomelo_check(!pomelo_map_has(map, key));

    pomelo_check(map->size == 1);

    // Destroy the map
    pomelo_map_destroy(map);
    
    // Check memleak
    pomelo_check(alloc_bytes == pomelo_allocator_allocated_bytes(allocator));
    return 0;
}
