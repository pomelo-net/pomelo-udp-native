#ifndef POMELO_UTILS_MAP_SRC_H
#define POMELO_UTILS_MAP_SRC_H
#include "array.h"
#include "list.h"
#include "mutex.h"


#ifdef __cplusplus
extern "C" {
#endif

/// The default map load factor
#define POMELO_MAP_DEFAULT_LOAD_FACTOR 0.75f

/// The default map initial number of buckets
#define POMELO_MAP_DEFAULT_INITIAL_BUCKETS 16

/// @brief The map entry
typedef struct pomelo_map_entry_s pomelo_map_entry_t;

/// @brief The map creating options
typedef struct pomelo_map_options_s pomelo_map_options_t;

/// @brief The map bucket
typedef struct pomelo_map_bucket_s pomelo_map_bucket_t;

/// @brief The map collection
typedef struct pomelo_map_s pomelo_map_t;

/// @brief The map iterator
typedef struct pomelo_map_iterator_s pomelo_map_iterator_t;

/// @brief Hash function of map
typedef int64_t (*pomelo_map_hash_fn)(
    pomelo_map_t * map,
    void * callback_context,
    void * p_key
);

/// @brief The compare function between two keys. It must return true if two
/// keys are equals
typedef bool (*pomelo_map_compare_fn)(
    pomelo_map_t * map,
    void * callback_context,
    void * p_first_key,
    void * p_second_key
);


struct pomelo_map_entry_s {
    /// @brief The map element key
    void * p_key;

    /// @brief The map element value
    void * p_value;
};


struct pomelo_map_bucket_s {
    /// @brief The list of elements in this bucket
    pomelo_list_t * entries;

    /// @brief The size of bucket (For redistributing purpose)
    size_t size;
};


struct pomelo_map_s {
    /// @brief The size of map (all elements)
    size_t size;

    /// @brief The allocator
    pomelo_allocator_t * allocator;

    /// @brief The hash function
    pomelo_map_hash_fn hash_fn;

    /// @brief The compare function
    pomelo_map_compare_fn compare_fn;

    /// @brief Value size
    size_t value_size;

    /// @brief Key size
    size_t key_size;

    /// @brief The number of buckets
    size_t nbuckets;

    /// @brief The dynamic array of buckets
    pomelo_array_t * buckets;

    /// @brief The pool of entries
    pomelo_pool_t * entry_pool;

    /// @brief The load factor of map
    float load_factor;

    /// @brief Initial number of buckets
    size_t initial_buckets;

    /// @brief Lock for synchronized map
    pomelo_mutex_t * mutex;

    /// @brief Modified count
    uint64_t mod_count;

    /// @brief Context for hashing and comparing functions
    void * callback_context;

#ifndef NDEBUG
    /// @brief The signature of map
    int signature;
#endif
};


struct pomelo_map_options_s {
    /// @brief The allocator
    pomelo_allocator_t * allocator;

    /// @brief The hash function If NULL is set, default hashing function
    /// will be used
    pomelo_map_hash_fn hash_fn;

    /// @brief The compare function. If NULL is set, default compare function
    /// will be used
    pomelo_map_compare_fn compare_fn;

    /// @brief Value size
    size_t value_size;

    /// @brief Key size
    size_t key_size;

    /// @brief The load factor of map (implemented as hash table)
    /// Default is 0.75.
    float load_factor;

    /// @brief Initial number of buckets. Default is 16.
    size_t initial_buckets;

    /// @brief Thread-safe option.
    /// Destroying the pool is not synchronized.
    bool synchronized;

    /// @brief The callback context for hashing and comparing functions
    void * callback_context;
};


struct pomelo_map_iterator_s {
    /// @brief The map of this iterator
    pomelo_map_t * map;

    /// @brief The current bucket index
    size_t bucket_index;

    /// @brief The current list node of entry
    pomelo_list_node_t * node;

    /// @brief Modified count when this iterator is started
    uint64_t mod_count;
};


/* -------------------------------------------------------------------------- */
/*                               Public APIs                                  */
/* -------------------------------------------------------------------------- */

/// @brief Set the default options for map
void pomelo_map_options_init(pomelo_map_options_t * options);

/// @brief Create new map
pomelo_map_t * pomelo_map_create(pomelo_map_options_t * options);

/// @brief Destroy a map
void pomelo_map_destroy(pomelo_map_t * map);

/// @brief Get a value from map (pointer version)
/// @return Returns 0 if entry is found or -1 if not
int pomelo_map_get_p(pomelo_map_t * map, void * p_key, void * p_value);

/// @brief Get the value from map
/// @return Returns 0 if entry is found or -1 if not
#define pomelo_map_get(map, key, p_value) pomelo_map_get_p(map, &(key), p_value)

/// @brief Check if key exists in the map (pointer version)
/// @return Returns true if key exists or false if not
bool pomelo_map_has_p(pomelo_map_t * map, void * p_key);

/// @brief Check if key exists in the map (pointer version)
/// @return Returns 1 if key exists or 0 if not
#define pomelo_map_has(map, key) pomelo_map_has_p(map, &(key))

/// @brief Set a value with key to map (Pointer version).
/// It will override existent key
/// @return Return 0 on success or -1 on failure
int pomelo_map_set_p(pomelo_map_t * map, void * p_key, void * p_value);

/// @brief Set a value with key to map.
/// @return Return 0 on success or -1 on failure
#define pomelo_map_set(map, key, value) pomelo_map_set_p(map, &(key), &(value))

/// @brief Delete a key from map (Pointer version)
/// @return Return 0 on success or -1 if key is not found
int pomelo_map_del_p(pomelo_map_t * map, void * p_key);

/// @brief Delete a key from map
/// @return Return 0 on success or -1 if key is not found
#define pomelo_map_del(map, key) pomelo_map_del_p(map, &(key))

/// @brief Clear all entries of map
void pomelo_map_clear(pomelo_map_t * map);

/// @brief Iterate over the map. This API is not threadsafe
void pomelo_map_iterate(pomelo_map_t * map, pomelo_map_iterator_t * it);

/// @brief Next map iteration.
/// Iterating modified map will return an error code.
/// @return 0 on success or -1 on finished or -2 if map has been
/// modified.
int pomelo_map_iterator_next(
    pomelo_map_iterator_t * it,
    pomelo_map_entry_t * entry
);

/// @brief Get the value of entry
#define pomelo_map_entry_value(entry, type) *((type*) (entry)->p_value)

/// @brief Get the value of entry. The type of value must be a pointer
#define pomelo_map_entry_value_ptr(entry) pomelo_map_entry_value(entry, void *)

#ifdef __cplusplus
}
#endif
#endif // POMELO_UTILS_MAP_SRC_H
