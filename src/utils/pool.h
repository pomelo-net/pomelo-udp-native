#ifndef POMELO_UTILS_POOL_SRC_H
#define POMELO_UTILS_POOL_SRC_H
#include <stdbool.h>
#include "pomelo/allocator.h"
#include "mutex.h"
#ifdef __cplusplus
extern "C" {
#endif


/// @brief The pool
typedef struct pomelo_pool_s pomelo_pool_t;

/// @brief The root pool
typedef struct pomelo_pool_root_s pomelo_pool_root_t;

/// @brief The pool element
typedef struct pomelo_pool_element_s pomelo_pool_element_t;

/// @brief The options for object pool
typedef struct pomelo_pool_root_options_s pomelo_pool_root_options_t;

/// @brief The shared pool.
/// This is a buffered pool which uses another pool as its data pool.
typedef struct pomelo_pool_shared_s pomelo_pool_shared_t;

/// @brief The shared pool options
typedef struct pomelo_pool_shared_options_s pomelo_pool_shared_options_t;

/// @brief The element callback for pool. If this function returns 0, the pool
/// process will continue or non-zero value will interrupt the process
typedef int (*pomelo_pool_alloc_cb)(void * element, void * alloc_data);

/// @brief The free callback for pool. This function will be called when the
/// element is freed.
typedef void (*pomelo_pool_free_cb)(void * element);

/// @brief The cleanup callback for pool. This function will be called when the
/// element is released.
typedef void (*pomelo_pool_cleanup_cb)(void * element);

/// @brief The initialize callback for pool. This function will be called when
/// the element is initialized.
typedef int (*pomelo_pool_init_cb)(void * element, void * init_data);


/// @brief The acquire function for pool. This function will be called when the
/// element is acquired.
typedef void * (*pomelo_pool_acquire_fn)(
    pomelo_pool_t * pool,
    void * init_data
);


/// @brief The release function for pool. This function will be called when the
/// element is released.
typedef void (*pomelo_pool_release_fn)(pomelo_pool_t * pool, void * element);


struct pomelo_pool_s {
    /// @brief The root pool
    pomelo_pool_root_t * root;

    /// @brief The acquire function
    pomelo_pool_acquire_fn acquire;

    /// @brief The release function
    pomelo_pool_release_fn release;
};


struct pomelo_pool_element_s {
    /// @brief Next element in available list
    pomelo_pool_element_t * available_next;

    /// @brief Previous element in available list
    pomelo_pool_element_t * available_prev;

    /// @brief Next element in allocated list
    pomelo_pool_element_t * allocated_next;

    /// @brief Previous element in allocated list
    pomelo_pool_element_t * allocated_prev;

    /// @brief The flags of elements.
    uint32_t flags;

#ifndef NDEBUG
    /// @brief The signature for debugging
    int signature;
#endif

    /* Hidden field: data */
};


struct pomelo_pool_root_s {
    /// @brief The base pool
    pomelo_pool_t base;

    /// @brief The allocator of pool
    pomelo_allocator_t * allocator;

    /// @brief The size of a single element (not the size of elements list)
    size_t element_size;

    /// @brief Maximum number of available elements
    size_t available_max;

    /// @brief Maximum number of allocated elements
    size_t allocated_max;

    /// @brief The head of pool (available elements)
    pomelo_pool_element_t * available_elements;

    /// @brief The head of all allocated elements
    pomelo_pool_element_t * allocated_elements;

    /// @brief Optional allocate callback
    pomelo_pool_alloc_cb on_alloc;

    /// @brief Optional free callback
    pomelo_pool_free_cb on_free;

    /// @brief Optional initialize callback
    pomelo_pool_init_cb on_init;

    /// @brief Optional cleanup callback
    pomelo_pool_cleanup_cb on_cleanup;

    /// @brief Data which is passed to the allocate callback
    void * alloc_data;

    /// @brief Size of allocated elements list
    size_t allocated_size;

    /// @brief Size of available elements list
    size_t available_size;

    /// @brief If this flag is set, the acquiring buffer will be initialize with
    /// zero values. Using this flag when setting either on_free or on_alloc
    /// is not allowed.
    bool zero_init;

    /// @brief The mutex lock for this pool.
    /// Only available for synchronized pool.
    /// This will be NULL if the options synchronized is not set.
    pomelo_mutex_t * mutex;

#ifndef NDEBUG
    /// @brief The signature for all pool
    int signature;

    /// @brief The signature of all elements belonging to this pool
    int element_signature;
#endif

};


struct pomelo_pool_root_options_s {
    /// @brief The size of one element
    size_t element_size;

    /// @brief Maximum number of available elements. Zero for unlimited.
    size_t available_max;

    /// @brief Maximum number of allocated elements. Zero for unlimited.
    size_t allocated_max;

    /// @brief The allocator for pool. If allocator is null, default allocator
    /// will be used
    pomelo_allocator_t * allocator;

    /// @brief Optional allocate callback. Content of newly allocated elements
    /// will be set to NULL before this callback is called.
    pomelo_pool_alloc_cb on_alloc;

    /// @brief Optional free callback
    pomelo_pool_free_cb on_free;

    /// @brief Optional initialize callback
    pomelo_pool_init_cb on_init;

    /// @brief Optional cleanup callback
    pomelo_pool_cleanup_cb on_cleanup;

    /// @brief Data which is passed to the allocate callback
    void * alloc_data;

    /// @brief If this flag is set, content of element will be set to NULL 
    /// before on_init callback is called.
    bool zero_init;

    /// @brief If this flag is set, the pool will become synchronized pool.
    /// It means that, acquiring & releasing will become atomic operators.
    /// This is needed for multi-threaded enviroment.
    /// Destroying the pool is not synchronized by this option. So that,
    /// destroying a lock-acquiring pool is undefined behavior.
    bool synchronized;
};


struct pomelo_pool_shared_s {
    /// @brief The base pool
    pomelo_pool_t base;

    /// @brief The allocator of this pool (Not its elements)
    pomelo_allocator_t * allocator;

    /// @brief The buffer size of pool
    size_t buffers;

    /// @brief The maximum size of elements array
    size_t nelements;

    /// @brief The array of current holding elements
    pomelo_pool_element_t ** elements;

    /// @brief The current index of elements array
    size_t index;

#ifndef NDEBUG
    /// @brief The signature for all shared pool
    int signature;

#endif

};


struct pomelo_pool_shared_options_s {
    /// @brief The allocator
    pomelo_allocator_t * allocator;

    /// @brief The referenced pool. This will be used to get the root pool.
    /// The root pool must be synchronized.
    pomelo_pool_t * origin_pool;

    /// @brief The buffer size of pool. Default is 16.
    /// If the number of elements in this pool decreases down to zero, it
    /// will acquire (N - buffers size) elements from the root pool.
    /// If the number of elements in this pool increases up to (2 * N), it will
    /// release (N) elements to the root pool.
    size_t buffers;
};


/* -------------------------------------------------------------------------- */
/*                               Public APIs                                  */
/* -------------------------------------------------------------------------- */

/// @brief Create new pool
pomelo_pool_t * pomelo_pool_root_create(pomelo_pool_root_options_t * options);


/// @brief Create new shared pool
pomelo_pool_t * pomelo_pool_shared_create(
    pomelo_pool_shared_options_t * options
);


/// @brief Destroy pool
void pomelo_pool_destroy(pomelo_pool_t * pool);


/// @brief Acquire a new element from pool with init_data passed to on_init
/// callback.
void * pomelo_pool_acquire(pomelo_pool_t * pool, void * init_data);


/// @brief Release an element to pool
void pomelo_pool_release(pomelo_pool_t * pool, void * data);


/// Get the number of in-use elements
#define pomelo_pool_in_use(pool)                                               \
    ((pool)->root->allocated_size - (pool)->root->available_size)



/* -------------------------------------------------------------------------- */
/*                               Private APIs                                 */
/* -------------------------------------------------------------------------- */


/// @brief Destroy the pool
void pomelo_pool_root_destroy(pomelo_pool_root_t * pool);


/// @brief Acquire an element from the pool
void * pomelo_pool_root_acquire(pomelo_pool_root_t * pool, void * init_data);


/// @brief Release an element to the pool
void pomelo_pool_root_release(pomelo_pool_root_t * pool, void * element);


/// @brief Destroy the shared pool.
/// All holding elements will be released to the master pool
void pomelo_pool_shared_destroy(pomelo_pool_shared_t * pool);


/// @brief Acquire an element with init_data passed to on_init callback
void * pomelo_shared_pool_acquire(
    pomelo_pool_shared_t * pool,
    void * init_data
);


/// @brief Release an element to this pool
void pomelo_pool_shared_release(pomelo_pool_shared_t * pool, void * element);


/// @brief Acquire elements from available list of pool
/// @return Number of actually acquired elements
size_t pomelo_pool_acquire_elements(
    pomelo_pool_root_t * pool,
    size_t nelements,
    pomelo_pool_element_t ** elements
);


/// @brief Allocate new elements and add them to the allocated list of pool
/// @return Number of actually allocated elements
size_t pomelo_pool_allocate_elements(
    pomelo_pool_root_t * pool,
    size_t nelements,
    pomelo_pool_element_t ** elements
);


/// @brief Release elements
void pomelo_pool_release_elements(
    pomelo_pool_root_t * pool,
    size_t nelements,
    pomelo_pool_element_t ** elements
);


/// @brief Link elements to allocated list
void pomelo_pool_link_allocated(
    pomelo_pool_root_t * pool,
    pomelo_pool_element_t ** elements,
    size_t nelements
);


/// @brief Unlink elements from allocated list
void pomelo_pool_unlink_allocated(
    pomelo_pool_root_t * pool,
    pomelo_pool_element_t ** elements,
    size_t nelements
);


/// @brief Link elements to available list
void pomelo_pool_link_available(
    pomelo_pool_root_t * pool,
    pomelo_pool_element_t ** elements,
    size_t nelements
);


/// @brief Unlink elements from available list
void pomelo_pool_unlink_available(
    pomelo_pool_root_t * pool,
    pomelo_pool_element_t ** elements,
    size_t nelements
);


#ifdef __cplusplus
}
#endif
#endif // POMELO_UTILS_POOL_SRC_H

