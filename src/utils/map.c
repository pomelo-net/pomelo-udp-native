#include <string.h>
#include <assert.h>
#include "map.h"


#ifndef NDEBUG // Debug mode

/// The signature of all maps
#define POMELO_MAP_SIGNATURE 0x553402

/// Check the signature of map in debug mode
#define pomelo_map_check_signature(map)                                        \
    assert((map)->signature == POMELO_MAP_SIGNATURE)

#else // Release mode

/// Check the signature of map in release mode, this is no-op
#define pomelo_map_check_signature(map) (void) map

#endif


/* -------------------------------------------------------------------------- */
/*                               Private APIs                                 */
/* -------------------------------------------------------------------------- */


/// @brief Resize the buckets of map
static bool pomelo_map_resize_buckets(pomelo_map_t * map, size_t new_nbuckets) {
    // Double the number of buckets
    size_t nbuckets = map->nbuckets;
    if (new_nbuckets <= nbuckets) {
        // Decrement of buckets has not been supported yet.
        return true;
    }

    if (pomelo_array_resize(map->buckets, new_nbuckets) < 0) {
        return false;
    }

    pomelo_map_bucket_t * bucket;
    pomelo_map_bucket_t * buckets = map->buckets->elements;
    pomelo_list_options_t options;
    options.allocator = map->allocator;
    options.element_size = sizeof(pomelo_map_entry_t *);
    for (size_t i = nbuckets; i < new_nbuckets; ++i) {
        bucket = buckets + i;
        bucket->entries = pomelo_list_create(&options);
        bucket->size = 0;
    }

    // Redistribute the buckets elements
    pomelo_map_hash_fn hash_func = map->hash_fn;
    void * callback_context = map->callback_context;
    for (size_t i = 0; i < nbuckets; ++i) {
        bucket = buckets + i;
        size_t bucket_size = bucket->size;
        pomelo_list_node_t * node = bucket->entries->front;
        pomelo_map_entry_t * entry;
        for (size_t j  = 0; j < bucket_size; j++) {
            entry = pomelo_list_element(node, pomelo_map_entry_t *);
            size_t hash = (size_t)
                hash_func(map, callback_context, entry->p_key);
            size_t index = hash % new_nbuckets;
            if (index != i) { 
                // The different bucket
                // Remove node from current list
                pomelo_list_remove(bucket->entries, node);
                // Redistributed bucket
                pomelo_list_push_back(buckets[index].entries, entry);
            }
            node = node->next;
        }
    }

    // Finally, synchronize the bucket size with the size of entries list
    for (size_t i = 0; i < new_nbuckets; ++i) {
        bucket = buckets + i;
        bucket->size = bucket->entries->size;
    }
    map->nbuckets = new_nbuckets;
    return true;
}


/// @brief Create new entry
static pomelo_map_entry_t * pomelo_map_create_entry(
    pomelo_map_t * map,
    void * p_key,
    void * p_value
) {
    void * callback_context = map->callback_context;
    size_t hash = (size_t) map->hash_fn(map, callback_context, p_key);
    size_t index = hash % map->nbuckets;
    pomelo_map_bucket_t * bucket = pomelo_array_get_p(map->buckets, index);
    assert(bucket != NULL);
    pomelo_map_compare_fn compare_fn = map->compare_fn;

    pomelo_map_entry_t * entry = NULL;
    pomelo_list_ptr_for(bucket->entries, entry, {
        if (compare_fn(map, callback_context, entry->p_key, p_key)) {
            // Found entry, update the value
            // Just the value is updated here, structure of map is not changed.
            // So that, we do not have to modify the modified count here.
            memcpy(entry->p_value, p_value, map->value_size);
            return entry;
        }
    });

    if (1.0f * (map->size + 1) / map->nbuckets > map->load_factor) {
        if (!pomelo_map_resize_buckets(map, map->nbuckets * 2)) {
            // Cannot increase the number of buckets
            return NULL;
        }
    }

    // The number of buckets has been changed and the bucket is redistributed,
    // so we need to find it again and update the modified count as weel
    index = hash % map->nbuckets;
    bucket = pomelo_array_get_p(map->buckets, index);

    // The entry has not existed, create new one
    entry = pomelo_pool_acquire(map->entry_pool);
    if (!entry) {
        return NULL;
    }

    // Update the entry layout
    entry->p_key = entry + 1;
    entry->p_value = ((uint8_t *) (entry + 1)) + map->value_size;

    memcpy(entry->p_key, p_key, map->key_size);
    memcpy(entry->p_value, p_value, map->value_size);

    pomelo_list_push_back(bucket->entries, entry);
    map->size++;
    map->mod_count++;
    bucket->size++;

    return entry;
}


/// @brief Remove an entry.
/// @returns Returns true if found entry, or false otherwise
static bool pomelo_map_remove_entry(pomelo_map_t * map, void * p_key) {
    void * callback_context = map->callback_context;
    size_t hash = (size_t) map->hash_fn(map, callback_context, p_key);
    size_t index = hash % map->nbuckets;
    pomelo_map_bucket_t * bucket = pomelo_array_get_p(map->buckets, index);

    pomelo_map_entry_t * entry;
    pomelo_list_node_t * node;

    pomelo_list_for_each(bucket->entries, node) {
        entry = pomelo_list_element(node, pomelo_map_entry_t *);

        if (map->compare_fn(map, callback_context, entry->p_key, p_key)) {
            // Remove entry from bucket and push it to pool
            pomelo_list_remove(bucket->entries, node);
            pomelo_pool_release(map->entry_pool, entry);
            map->size--;
            bucket->size--;
            map->mod_count++; // Update the modified count
            return true;
        }
    }

    // Not found entry
    return false;
}


/// @brief Find entry
static pomelo_map_entry_t * pomelo_map_find_entry(
    pomelo_map_t * map,
    void * p_key
) {
    if (map->nbuckets == 0) {
        // There's no bucket, not found the entry
        return NULL;
    }

    void * callback_context = map->callback_context;
    size_t hash = (size_t) map->hash_fn(map, callback_context, p_key);
    size_t index = hash % map->nbuckets;
    pomelo_map_bucket_t * bucket = pomelo_array_get_p(map->buckets, index);
    assert(bucket != NULL);

    pomelo_map_entry_t * entry;
    pomelo_list_ptr_for(bucket->entries, entry, {
        if (map->compare_fn(map, callback_context, entry->p_key, p_key)) {
            return entry;
        }
    });

    // Not found the entry
    return NULL;
}


/// @brief Clear all entries of map
static void pomelo_map_clear_entries(pomelo_map_t * map) {
    pomelo_array_t * buckets = map->buckets;
    size_t bucket_size = buckets->size;
    for (size_t i = 0; i < bucket_size; ++i) {
        pomelo_map_bucket_t * bucket = pomelo_array_get_p(buckets, i);
        pomelo_map_entry_t * entry;
        pomelo_list_ptr_for(bucket->entries, entry, {
            pomelo_pool_release(map->entry_pool, entry);
        });

        pomelo_list_clear(bucket->entries);
        bucket->size = 0;
    }

    map->size = 0;
    map->mod_count++;
}


/* Some default key hashing & comparing functions */
#define pomelo_map_hash_x(base)                 \
static int64_t pomelo_map_hash_##base(          \
    pomelo_map_t * map,                         \
    void * callback_context,                    \
    uint##base##_t * p_key                      \
) {                                             \
    (void) map;                                 \
    (void) callback_context;                    \
    return (int64_t) (*p_key);                  \
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

static bool pomelo_map_compare_common(
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


/* -------------------------------------------------------------------------- */
/*                               Public APIs                                  */
/* -------------------------------------------------------------------------- */


void pomelo_map_options_init(pomelo_map_options_t * options) {
    assert(options != NULL);
    memset(options, 0, sizeof(pomelo_map_options_t));
}


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
            compare_fn = (pomelo_map_compare_fn) pomelo_map_compare_common;
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
    map->nbuckets = 0;
    map->load_factor = options->load_factor > 0
        ? options->load_factor
        : POMELO_MAP_DEFAULT_LOAD_FACTOR;

#ifndef NDEBUG
    map->signature = POMELO_MAP_SIGNATURE;
#endif

    size_t initial_buckets = options->initial_buckets > 0
        ? options->initial_buckets
        : POMELO_MAP_DEFAULT_INITIAL_BUCKETS;

    // Create buckets
    pomelo_array_options_t buckets_options;
    pomelo_array_options_init(&buckets_options);
    buckets_options.allocator = allocator;
    buckets_options.element_size = sizeof(pomelo_map_bucket_t);
    buckets_options.initial_capacity = initial_buckets;

    map->buckets = pomelo_array_create(&buckets_options);
    if (!map->buckets) {
        pomelo_map_destroy(map);
        return NULL;
    }

    // Create entry pool
    pomelo_pool_options_t entry_pool_options;
    pomelo_pool_options_init(&entry_pool_options);
    entry_pool_options.allocator = allocator;
    entry_pool_options.element_size =
        sizeof(pomelo_map_entry_t) + options->key_size + options->value_size;

    map->entry_pool = pomelo_pool_create(&entry_pool_options);
    if (!map->entry_pool) {
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
        pomelo_map_bucket_t * buckets = map->buckets->elements;
        for (size_t i = 0; i < map->nbuckets; ++i) {
            pomelo_list_destroy(buckets[i].entries);
        }
        pomelo_array_destroy(map->buckets);
        map->buckets = NULL;
    }

    // Destroy entry pool
    if (map->entry_pool) {
        pomelo_pool_destroy(map->entry_pool);
        map->entry_pool = NULL;
    }

    // Destroy mutex lock
    if (map->mutex) {
        pomelo_mutex_destroy(map->mutex);
        map->mutex = NULL;
    }

    // Free itself
    pomelo_allocator_free(map->allocator, map);
}


int pomelo_map_get_p(pomelo_map_t * map, void * p_key, void * p_value) {
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


bool pomelo_map_has_p(pomelo_map_t * map, void * p_key) {
    assert(map != NULL);
    assert(p_key != NULL);
    pomelo_map_check_signature(map);

    pomelo_mutex_t * mutex = map->mutex;
    POMELO_BEGIN_CRITICAL_SECTION(mutex);

    pomelo_map_entry_t * entry = pomelo_map_find_entry(map, p_key);

    POMELO_END_CRITICAL_SECTION(mutex);
    return entry != NULL;
}


int pomelo_map_set_p(pomelo_map_t * map, void * p_key, void * p_value) {
    assert(map != NULL);
    assert(p_key != NULL);
    assert(p_value != NULL);
    pomelo_map_check_signature(map);

    pomelo_mutex_t * mutex = map->mutex;
    POMELO_BEGIN_CRITICAL_SECTION(mutex);

    pomelo_map_entry_t * entry = pomelo_map_create_entry(map, p_key, p_value);

    POMELO_END_CRITICAL_SECTION(mutex);
    return entry != NULL ? 0 : -1;
}


int pomelo_map_del_p(pomelo_map_t * map, void * p_key) {
    assert(map != NULL);
    assert(p_key != NULL);
    pomelo_map_check_signature(map);

    pomelo_mutex_t * mutex = map->mutex;
    POMELO_BEGIN_CRITICAL_SECTION(mutex);

    bool ret = pomelo_map_remove_entry(map, p_key);

    POMELO_END_CRITICAL_SECTION(mutex);
    return ret ? 0 : -1;
}


void pomelo_map_clear(pomelo_map_t * map) {
    assert(map != NULL);
    pomelo_map_check_signature(map);

    pomelo_mutex_t * mutex = map->mutex;
    POMELO_BEGIN_CRITICAL_SECTION(mutex);

    pomelo_map_clear_entries(map);

    POMELO_END_CRITICAL_SECTION(mutex);
}


void pomelo_map_iterate(pomelo_map_t * map, pomelo_map_iterator_t * it) {
    assert(map != NULL);
    assert(it != NULL);
    pomelo_map_check_signature(map);

    pomelo_mutex_t * mutex = map->mutex;
    pomelo_array_t * buckets = map->buckets;

    POMELO_BEGIN_CRITICAL_SECTION(mutex);

    it->map = map;
    it->node = NULL;
    it->bucket_index = 0;
    it->mod_count = map->mod_count;

    if (map->size > 0) {
        for (size_t i = 0; i < buckets->size; ++i) {
            pomelo_map_bucket_t * bucket = pomelo_array_get_p(buckets, i);
            if (bucket->size > 0) {
                // Found first non-empty bucket
                it->bucket_index = i;
                it->node = bucket->entries->front;
                break;
            }
        }
    }

    POMELO_END_CRITICAL_SECTION(mutex);
}


int pomelo_map_iterator_next(
    pomelo_map_iterator_t * it,
    pomelo_map_entry_t * entry
) {
    assert(it != NULL);
    assert(entry != NULL);

    pomelo_mutex_t * mutex = it->map->mutex;
    pomelo_array_t * buckets = it->map->buckets;
    int ret = 0;

    POMELO_BEGIN_CRITICAL_SECTION(mutex);
    do {
        if (!it->node) {
            // Iteration has been finished
            ret = -1;
            break; // do-while
        }

        if (it->mod_count != it->map->mod_count) {
            // Map has changed
            ret = -2;
            break; // do-while
        }

        *entry = *pomelo_list_element(it->node, pomelo_map_entry_t *);
        it->node = it->node->next;
        if (it->node) { // Found the next nnode
            break; // do-while
        }

        // Try to find next node
        for (size_t i = it->bucket_index + 1; i < buckets->size; i++) {
            pomelo_map_bucket_t * bucket = pomelo_array_get_p(buckets, i);
            if (bucket->size > 0) {
                it->bucket_index = i;
                it->node = bucket->entries->front;
                break; // for-loop
            }
        }
    } while (0);

    POMELO_END_CRITICAL_SECTION(mutex);
    return ret;
}
