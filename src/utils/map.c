#include <string.h>
#include <assert.h>
#include "map.h"


#ifndef NDEBUG // Debug mode

/// The signature of all maps
#define POMELO_MAP_SIGNATURE 0x553402

/// Check the signature of map in debug mode
#define pomelo_map_check_signature(map)                                        \
    assert((map)->signature == POMELO_MAP_SIGNATURE)

/// Check the signature of entry in debug mode
#define pomelo_map_entry_check_signature(map, entry)                           \
    assert((entry)->signature == (map)->entry_signature)

/// The signature generator of map entries
static int map_entry_signature_generator = 0;

#else // Release mode

/// Check the signature of map in release mode, this is no-op
#define pomelo_map_check_signature(map)

/// Check the signature of entry in release mode, this is no-op
#define pomelo_map_entry_check_signature(map, entry)

#endif


/* Some default key hashing & comparing functions */
#define pomelo_map_hash_x(base)                 \
static size_t pomelo_map_hash_##base(           \
    pomelo_map_t * map,                         \
    void * callback_context,                    \
    uint##base##_t * p_key                      \
) {                                             \
    (void) map;                                 \
    (void) callback_context;                    \
    return (size_t) (*p_key);                   \
}

#define pomelo_map_compare_x(base)              \
static bool pomelo_map_compare_##base(          \
    pomelo_map_t * map,                         \
    void * callback_context,                    \
    uint##base##_t * p_first_key,               \
    uint##base##_t * p_second_key               \
) {                                             \
    (void) map;                                 \
    (void) callback_context;                    \
    assert(p_first_key != NULL);                \
    assert(p_second_key != NULL);               \
    return (*p_first_key) == (*p_second_key);   \
}


pomelo_map_hash_x(8)
pomelo_map_hash_x(16)
pomelo_map_hash_x(32)
pomelo_map_hash_x(64)
pomelo_map_compare_x(8)
pomelo_map_compare_x(16)
pomelo_map_compare_x(32)
pomelo_map_compare_x(64)


/// @brief Compare two keys
static bool pomelo_map_common_compare(
    pomelo_map_t * map,
    void * callback_context,
    void * p_first_key,
    void * p_second_key
) {
    assert(map != NULL);
    (void) callback_context;
    assert(p_first_key != NULL);
    assert(p_second_key != NULL);
    return memcmp(p_first_key, p_second_key, map->key_size) == 0;
}


/// @brief Make the memory layout of entry
static int entry_on_alloc(pomelo_map_entry_t * entry, pomelo_map_t * map) {
    assert(entry != NULL);
    assert(map != NULL);

    entry->p_key = entry + 1;
    entry->p_value = ((uint8_t *) entry->p_key) + map->key_size;

    return 0;
}


/* -------------------------------------------------------------------------- */
/*                               Public APIs                                  */
/* -------------------------------------------------------------------------- */

pomelo_map_t * pomelo_map_create(pomelo_map_options_t * options) {
    assert(options != NULL);
    size_t key_size = options->key_size;
    if (options->value_size == 0 || key_size == 0) {
        // Invalid key and value size
        return NULL;
    }

    // Check hash function
    pomelo_map_hash_fn hash_fn = options->hash_fn;
    if (!hash_fn) {
        if (key_size >= sizeof(uint64_t)) {
            hash_fn = (pomelo_map_hash_fn) pomelo_map_hash_64;
        } else if (key_size >= sizeof(uint32_t)) {
            hash_fn = (pomelo_map_hash_fn) pomelo_map_hash_32;
        } else if (key_size >= sizeof(uint16_t)) {
            hash_fn = (pomelo_map_hash_fn) pomelo_map_hash_16;
        } else {
            hash_fn = (pomelo_map_hash_fn) pomelo_map_hash_8;
        }
    }

    // Check compare function
    pomelo_map_compare_fn compare_fn = options->compare_fn;
    if (!compare_fn) {
        if (key_size == sizeof(uint64_t)) {
            compare_fn = (pomelo_map_compare_fn) pomelo_map_compare_64;
        } else if (key_size == sizeof(uint32_t)) {
            compare_fn = (pomelo_map_compare_fn) pomelo_map_compare_32;
        } else if (key_size == sizeof(uint16_t)) {
            compare_fn = (pomelo_map_compare_fn) pomelo_map_compare_16;
        } else if (key_size == sizeof(uint8_t)) {
            compare_fn = (pomelo_map_compare_fn) pomelo_map_compare_8;
        } else {
            compare_fn = (pomelo_map_compare_fn) pomelo_map_common_compare;
        }
    }

    pomelo_allocator_t * allocator = options->allocator;
    if (!allocator) {
        allocator = pomelo_allocator_default();
    }

    pomelo_map_t * map = pomelo_allocator_malloc_t(allocator, pomelo_map_t);
    if (!map) {
        return NULL;
    }

    memset(map, 0, sizeof(pomelo_map_t));

    map->allocator = allocator;
    map->hash_fn = hash_fn;
    map->compare_fn = compare_fn;
    map->callback_context = options->callback_context;
    map->value_size = options->value_size;
    map->key_size = options->key_size;
    map->load_factor = options->load_factor > 0
        ? options->load_factor
        : POMELO_MAP_DEFAULT_LOAD_FACTOR;

#ifndef NDEBUG
    map->signature = POMELO_MAP_SIGNATURE;
    map->entry_signature = map_entry_signature_generator++;
#endif

    size_t initial_buckets = options->initial_buckets > 0
        ? options->initial_buckets
        : POMELO_MAP_DEFAULT_INITIAL_BUCKETS;

    // Create buckets
    pomelo_array_options_t array_options = {
        .allocator = allocator,
        .element_size = sizeof(pomelo_map_bucket_t *),
        .initial_capacity = initial_buckets
    };
    map->buckets = pomelo_array_create(&array_options);
    if (!map->buckets) {
        pomelo_map_destroy(map);
        return NULL;
    }

    // Create entry pool
    pomelo_pool_root_options_t pool_options;
    memset(&pool_options, 0, sizeof(pomelo_pool_root_options_t));
    pool_options.allocator = allocator;
    pool_options.element_size =
        (sizeof(pomelo_map_entry_t) + options->key_size + options->value_size);
    pool_options.on_alloc = (pomelo_pool_alloc_cb) entry_on_alloc;
    pool_options.alloc_data = map;
    map->entry_pool = pomelo_pool_root_create(&pool_options);
    if (!map->entry_pool) {
        pomelo_map_destroy(map);
        return NULL;
    }

    // Create bucket pool
    memset(&pool_options, 0, sizeof(pomelo_pool_root_options_t));
    pool_options.allocator = allocator;
    pool_options.element_size = sizeof(pomelo_map_bucket_t);
    map->bucket_pool = pomelo_pool_root_create(&pool_options);
    if (!map->bucket_pool) {
        pomelo_map_destroy(map);
        return NULL;
    }

    // Create mutex lock
    if (options->synchronized) {
        // This map is synchronized.
        map->mutex = pomelo_mutex_create(allocator);
        if (!map->mutex) {
            pomelo_map_destroy(map);
            return NULL;
        }
    }

    // Create bucket entries context
    pomelo_list_context_options_t context_options = {
        .allocator = allocator,
        .element_size = sizeof(pomelo_map_entry_t *)
    };
    map->bucket_entries_context = pomelo_list_context_create(&context_options);
    if (!map->bucket_entries_context) {
        pomelo_map_destroy(map);
        return NULL;
    }

    // Increase the number of buckets by initial buckets
    if (!pomelo_map_resize_buckets(map, initial_buckets)) {
        // Cannot create initial buckets
        pomelo_map_destroy(map);
        return NULL;
    }

    return map;
}


void pomelo_map_destroy(pomelo_map_t * map) {
    assert(map != NULL);
    pomelo_map_check_signature(map);

    // Destroy buckets
    if (map->buckets) {
        pomelo_array_t * buckets = map->buckets;
        for (size_t i = 0; i < buckets->size; ++i) {
            pomelo_map_bucket_t * bucket = NULL;
            pomelo_array_get(buckets, i, &bucket);
            if (bucket) {
                pomelo_map_bucket_cleanup(bucket);
            }
        }

        pomelo_array_destroy(map->buckets);
        map->buckets = NULL;
    }

    // Destroy entry pool
    if (map->entry_pool) {
        pomelo_pool_destroy(map->entry_pool);
        map->entry_pool = NULL;
    }

    // Destroy bucket pool
    if (map->bucket_pool) {
        pomelo_pool_destroy(map->bucket_pool);
        map->bucket_pool = NULL;
    }

    // Destroy mutex lock
    if (map->mutex) {
        pomelo_mutex_destroy(map->mutex);
        map->mutex = NULL;
    }

    // Destroy bucket entries context
    if (map->bucket_entries_context) {
        pomelo_list_context_destroy(map->bucket_entries_context);
        map->bucket_entries_context = NULL;
    }

    // Free itself
    pomelo_allocator_free(map->allocator, map);
}


int pomelo_map_get_ptr(pomelo_map_t * map, void * p_key, void * p_value) {
    assert(map != NULL);
    assert(p_key != NULL);
    assert(p_value != NULL);
    pomelo_map_check_signature(map);

    pomelo_mutex_t * mutex = map->mutex;
    POMELO_BEGIN_CRITICAL_SECTION(mutex);

    pomelo_map_entry_t * entry = pomelo_map_find_entry(map, p_key);

    POMELO_END_CRITICAL_SECTION(mutex);
    if (!entry) {
        // Not found the entry
        return -1;
    }

    memcpy(p_value, entry->p_value, map->value_size);
    return 0;
}


bool pomelo_map_has_ptr(pomelo_map_t * map, void * p_key) {
    assert(map != NULL);
    assert(p_key != NULL);
    pomelo_map_check_signature(map);

    pomelo_mutex_t * mutex = map->mutex;
    POMELO_BEGIN_CRITICAL_SECTION(mutex);

    pomelo_map_entry_t * entry = pomelo_map_find_entry(map, p_key);

    POMELO_END_CRITICAL_SECTION(mutex);
    return entry != NULL;
}


pomelo_map_entry_t * pomelo_map_set_ptr(
    pomelo_map_t * map,
    void * p_key,
    void * p_value
) {
    assert(map != NULL);
    assert(p_key != NULL);
    assert(p_value != NULL);
    pomelo_map_check_signature(map);

    pomelo_mutex_t * mutex = map->mutex;
    POMELO_BEGIN_CRITICAL_SECTION(mutex);

    pomelo_map_entry_t * entry = pomelo_map_set_entry(map, p_key, p_value);

    POMELO_END_CRITICAL_SECTION(mutex);
    return entry;
}


int pomelo_map_del_ptr(pomelo_map_t * map, void * p_key) {
    assert(map != NULL);
    assert(p_key != NULL);
    pomelo_map_check_signature(map);

    pomelo_mutex_t * mutex = map->mutex;
    POMELO_BEGIN_CRITICAL_SECTION(mutex);

    pomelo_map_entry_t * entry = pomelo_map_find_entry(map, p_key);
    if (!entry) {
        // Not found the entry
        POMELO_END_CRITICAL_SECTION(mutex);
        return -1;
    }

    pomelo_map_del_entry(map, entry);

    POMELO_END_CRITICAL_SECTION(mutex);
    return 0;
}


void pomelo_map_remove(pomelo_map_t * map, pomelo_map_entry_t * entry) {
    assert(entry != NULL);
    pomelo_map_del_ptr(map, entry->p_key);
}


void pomelo_map_clear(pomelo_map_t * map) {
    assert(map != NULL);
    pomelo_map_check_signature(map);

    pomelo_mutex_t * mutex = map->mutex;
    POMELO_BEGIN_CRITICAL_SECTION(mutex);

    pomelo_map_clear_entries(map);

    POMELO_END_CRITICAL_SECTION(mutex);
}


void pomelo_map_iterator_init(
    pomelo_map_iterator_t * it,
    pomelo_map_t * map
) {
    assert(map != NULL);
    assert(it != NULL);
    pomelo_map_check_signature(map);

    pomelo_mutex_t * mutex = map->mutex;
    pomelo_array_t * buckets = map->buckets;

    POMELO_BEGIN_CRITICAL_SECTION(mutex);

    it->map = map;
    it->entry = NULL;
    it->bucket_index = 0;
    it->mod_count = map->mod_count;

    if (map->size > 0) {
        size_t bucket_size = buckets->size;
        for (size_t i = 0; i < bucket_size; ++i) {
            pomelo_map_bucket_t * bucket = NULL;
            pomelo_array_get(buckets, i, &bucket);
            assert(bucket != NULL);

            if (bucket->entries->size > 0) {
                // Found first non-empty bucket
                it->bucket_index = i;
                it->entry = bucket->entries->front;
                break;
            }
        }
    }

    POMELO_END_CRITICAL_SECTION(mutex);
}


int pomelo_map_iterator_next(
    pomelo_map_iterator_t * it,
    pomelo_map_entry_t ** p_entry
) {
    assert(it != NULL);
    assert(p_entry != NULL);

    pomelo_mutex_t * mutex = it->map->mutex;
    pomelo_array_t * buckets = it->map->buckets;
    int ret = 0;

    POMELO_BEGIN_CRITICAL_SECTION(mutex);
    do {
        if (it->mod_count != it->map->mod_count) {
            assert(false); // Mod count has changed
            ret = -2;
            break; // do-while
        }

        if (!it->entry) {
            // Iteration has been finished
            ret = -1;
            break; // do-while
        }

        *p_entry = pomelo_list_element(it->entry, pomelo_map_entry_t *);
        it->entry = it->entry->next;
        if (it->entry) { // Found the next nnode
            break; // do-while
        }

        // Try to find next node
        for (size_t i = it->bucket_index + 1; i < buckets->size; i++) {
            pomelo_map_bucket_t * bucket = NULL;
            pomelo_array_get(buckets, i, &bucket);
            assert(bucket != NULL);

            if (bucket->entries->size > 0) {
                it->bucket_index = i;
                it->entry = bucket->entries->front;
                break; // for-loop
            }
        }
    } while (0);

    POMELO_END_CRITICAL_SECTION(mutex);
    return ret;
}


/* -------------------------------------------------------------------------- */
/*                              Internal APIs                                 */
/* -------------------------------------------------------------------------- */

bool pomelo_map_resize_buckets(pomelo_map_t * map, size_t new_nbuckets) {
    // Double the number of buckets
    size_t prev_nbuckets = map->buckets->size;
    if (new_nbuckets <= prev_nbuckets) {
        return true; // Decrement of buckets has not been supported yet.
    }

    // Resize the buckets
    if (pomelo_array_resize(map->buckets, new_nbuckets) < 0) {
        return false; // Failed to resize the buckets
    }

    // Initialize the new buckets
    pomelo_array_t * buckets = map->buckets;
    pomelo_pool_t * bucket_pool = map->bucket_pool;
    bool success = true;
    for (size_t i = prev_nbuckets; i < new_nbuckets; ++i) {
        pomelo_map_bucket_t * bucket = pomelo_pool_acquire(bucket_pool, NULL);
        if (!bucket) {
            success = false;
            break; // Failed to acquire the bucket
        }

        int ret = pomelo_map_bucket_init(bucket, map);
        if (ret < 0) {
            pomelo_pool_release(bucket_pool, bucket);
            success = false;
            break; // Failed to initialize the bucket
        }

        pomelo_array_set(buckets, i, bucket);
    }

    if (!success) {
        // Revert the resize operation
        for (size_t i = prev_nbuckets; i < new_nbuckets; i++) {
            pomelo_map_bucket_t * bucket = NULL;
            pomelo_array_get(buckets, i, &bucket);
            if (bucket) {
                pomelo_map_bucket_cleanup(bucket);
                pomelo_pool_release(bucket_pool, bucket);
            }
        }

        pomelo_array_resize(buckets, prev_nbuckets);
        return false;
    }

    // Redistribute the buckets elements
    pomelo_map_bucket_t * bucket = NULL;
    for (size_t i = 0; i < prev_nbuckets; i++) {
        // Non-error operations
        pomelo_array_get(buckets, i, &bucket);
        pomelo_map_bucket_redistribute(bucket);
    }

    return true;
}


pomelo_map_entry_t * pomelo_map_set_entry(
    pomelo_map_t * map,
    void * p_key,
    void * p_value
) {
    void * callback_context = map->callback_context;
    size_t nbuckets = map->buckets->size;
    size_t hash = (size_t) map->hash_fn(map, callback_context, p_key);
    size_t index = hash % nbuckets;
    pomelo_map_compare_fn compare_fn = map->compare_fn;

    // Get the bucket
    pomelo_map_bucket_t * bucket = NULL;
    pomelo_array_get(map->buckets, index, &bucket);
    assert(bucket != NULL);

    // Find the entry
    pomelo_map_entry_t * entry = NULL;
    pomelo_list_iterator_t it;
    pomelo_list_iterator_init(&it, bucket->entries);
    while (pomelo_list_iterator_next(&it, &entry) == 0) {
        if (compare_fn(map, callback_context, entry->p_key, p_key)) {
            // Found entry, update the value
            // Just the value is updated here, structure of map is not changed.
            // So that, we do not have to modify the modified count here.
            memcpy(entry->p_value, p_value, map->value_size);
            return entry;
        }
    }

    // Check the load factor
    if ((1.0f * (map->size + 1) / nbuckets) > map->load_factor) {
        if (!pomelo_map_resize_buckets(map, nbuckets * 2)) {
            return NULL; // Cannot increase the number of buckets
        }

        // The number of buckets has been changed and the bucket is
        // redistributed, so we need to find it again and update the modified
        // count as well.
        index = hash % nbuckets;
        pomelo_array_get(map->buckets, index, &bucket);
        assert(bucket != NULL);
    }

    // The entry has not existed, create new one
    entry = pomelo_pool_acquire(map->entry_pool, NULL);
    if (!entry) return NULL; // Failed to create new entry

    memcpy(entry->p_key, p_key, map->key_size);
    memcpy(entry->p_value, p_value, map->value_size);

#ifndef NDEBUG
    entry->signature = map->entry_signature;
#endif

    entry->bucket = bucket;
    entry->bucket_entry = pomelo_list_push_back(bucket->entries, entry);
    if (!entry->bucket_entry) {
        pomelo_pool_release(map->entry_pool, entry);
        return NULL; // Failed to push back the entry
    }

    map->size++;
    map->mod_count++;

    return entry;
}


void pomelo_map_del_entry(pomelo_map_t * map, pomelo_map_entry_t * entry) {
    assert(entry != NULL);
    pomelo_map_check_signature(map);
    pomelo_map_entry_check_signature(map, entry);

    pomelo_map_bucket_t * bucket = entry->bucket;
    pomelo_list_entry_t * bucket_entry = entry->bucket_entry;
    entry->bucket_entry = NULL;

    pomelo_list_remove(bucket->entries, bucket_entry);
    pomelo_pool_release(map->entry_pool, entry);

    map->size--;
    map->mod_count++;
}


pomelo_map_entry_t * pomelo_map_find_entry(pomelo_map_t * map, void * p_key) {
    if (map->buckets->size == 0) {
        return NULL; // There's no bucket, not found the entry
    }

    void * context = map->callback_context;
    size_t hash = (size_t) map->hash_fn(map, context, p_key);
    size_t index = hash % map->buckets->size;

    pomelo_map_bucket_t * bucket = NULL;
    pomelo_array_get(map->buckets, index, &bucket);
    assert(bucket != NULL);

    pomelo_map_entry_t * entry = NULL;
    pomelo_map_compare_fn compare_fn = map->compare_fn;
    pomelo_list_iterator_t it;
    pomelo_list_iterator_init(&it, bucket->entries);
    while (pomelo_list_iterator_next(&it, &entry) == 0) {
        if (compare_fn(map, context, entry->p_key, p_key)) {
            return entry; // Found the entry
        }
    }

    // Not found the entry
    return NULL;
}


void pomelo_map_clear_entries(pomelo_map_t * map) {
    pomelo_array_t * buckets = map->buckets;
    size_t bucket_size = buckets->size;
    pomelo_map_bucket_t * bucket;
    for (size_t i = 0; i < bucket_size; ++i) {
        pomelo_array_get(buckets, i, &bucket);
        pomelo_map_bucket_clear_entries(bucket);
    }

    map->size = 0;
    map->mod_count++;
}


int pomelo_map_bucket_init(pomelo_map_bucket_t * bucket, pomelo_map_t * map) {
    assert(bucket != NULL);
    assert(map != NULL);

    bucket->map = map;

    pomelo_list_options_t list_options = {
        .allocator = map->allocator,
        .element_size = sizeof(pomelo_map_entry_t *),
        .context = map->bucket_entries_context
    };
    bucket->entries = pomelo_list_create(&list_options);
    if (!bucket->entries) {
        pomelo_map_bucket_cleanup(bucket);
        return -1; // Failed to create the bucket entries list
    }

    return 0;
}


void pomelo_map_bucket_cleanup(pomelo_map_bucket_t * bucket) {
    assert(bucket != NULL);
    pomelo_map_bucket_clear_entries(bucket);

    if (bucket->entries) {
        pomelo_list_destroy(bucket->entries);
        bucket->entries = NULL;
    }
}


void pomelo_map_bucket_clear_entries(pomelo_map_bucket_t * bucket) {
    assert(bucket != NULL);

    pomelo_pool_t * entry_pool = bucket->map->entry_pool;
    pomelo_map_entry_t * entry;
    while (pomelo_list_pop_front(bucket->entries, &entry) == 0) {
        pomelo_pool_release(entry_pool, entry);
    }
}


void pomelo_map_bucket_redistribute(pomelo_map_bucket_t * bucket) {
    assert(bucket != NULL);

    pomelo_map_t * map = bucket->map;

    pomelo_map_hash_fn hash_func = map->hash_fn;
    void * callback_context = map->callback_context;
    pomelo_array_t * buckets = map->buckets;
    size_t nbuckets = buckets->size;

    pomelo_list_iterator_t it;
    pomelo_list_iterator_init(&it, bucket->entries);
    pomelo_map_entry_t * map_entry;
    while (pomelo_list_iterator_next(&it, &map_entry) == 0) {
        size_t hash = hash_func(map, callback_context, map_entry->p_key);
        size_t index = hash % nbuckets;
        pomelo_map_bucket_t * new_bucket = NULL;
        pomelo_array_get(buckets, index, &new_bucket);
        if (new_bucket == bucket) continue; // Same bucket

        // Change the bucket of entry
        map_entry->bucket = new_bucket;
        map_entry->bucket_entry =
            pomelo_list_iterator_transfer(&it, new_bucket->entries);
        assert(map_entry->bucket_entry != NULL);
    }
}
