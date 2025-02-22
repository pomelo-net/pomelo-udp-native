#ifndef POMELO_UTILS_POOL_SRC_H
#define POMELO_UTILS_POOL_SRC_H
#include <stdbool.h>
#include "pomelo/allocator.h"
#include "mutex.h"
#ifdef __cplusplus
extern "C" {
#endif


/// @brief The object pool implemented as a stack
typedef struct pomelo_pool_s pomelo_pool_t;

/// @brief The pool element
typedef struct pomelo_pool_element_s pomelo_pool_element_t;

/// @brief The options for object pool
typedef struct pomelo_pool_options_s pomelo_pool_options_t;

/// @brief The shared pool.
/// This is a buffered pool which uses another pool as its data pool.
typedef struct pomelo_shared_pool_s pomelo_shared_pool_t;

/// @brief The shared pool options
typedef struct pomelo_shared_pool_options_s pomelo_shared_pool_options_t;

/// @brief The element callback for pool. If this function returns 0, the pool
/// process will continue or non-zero value will interrupt the process
typedef int (*pomelo_pool_callback)(void * element, void * context);


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
    uint8_t flags;

#ifndef NDEBUG
    /// @brief The signature for debugging
    int signature;
#endif

    /* Hidden field: data */
};

struct pomelo_pool_s {
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

    /// @brief The initialize callback. This callback will be called once
    /// right after the element is allocated.
    pomelo_pool_callback allocate_callback;

    /// @brief The finalize callback. This callback will be called once right
    /// before the element is freed (In destroy function of pool)
    pomelo_pool_callback deallocate_callback;

    /// @brief Release callback. This callback is called every release call.
    /// The return value of this function will be ignored.
    pomelo_pool_callback release_callback;

    /// @brief Acquire callback. This callback is called every acquire call.
    /// The return value of this function will be ignored.
    pomelo_pool_callback acquire_callback;

    /// @brief The context for element callbacks
    void * callback_context;

    /// @brief Size of allocated elements list
    size_t allocated_size;

    /// @brief Size of available elements list
    size_t available_size;

    /// @brief If this flag is set, the acquiring buffer will be initialize with
    /// zero values. Using this flag when setting either alloc_callback or
    /// dealloc_callback is probihited.
    bool zero_initialized;

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

struct pomelo_pool_options_s {
    /// @brief The size of one element
    size_t element_size;

    /// @brief Maximum number of available elements. Zero for unlimited.
    size_t available_max;

    /// @brief Maximum number of allocated elements. Zero for unlimited.
    size_t allocated_max;

    /// @brief The allocator for pool. If allocator is null, default allocator
    /// will be used
    pomelo_allocator_t * allocator;

    /// @brief The initialize callback. This callback will be called once
    /// right after the element is allocated. 
    /// If NULL is set, no callback will be called.
    pomelo_pool_callback allocate_callback;

    /// @brief The finalize callback. This callback will be called once right
    /// before the element is freed (In destroy function of pool)
    /// If NULL is set, no callback will be called.
    pomelo_pool_callback deallocate_callback;

    /// @brief Release callback. If NULL is set, no callback will be called.
    /// The return value of this function will be ignored.
    pomelo_pool_callback release_callback;

    /// @brief Acquire callback. If NULL is set, no callback will be called.
    /// The return value of this function will be ignored.
    pomelo_pool_callback acquire_callback;

    /// @brief The callback context
    void * callback_context;

    /// @brief If this flag is set, the acquiring buffer will be initialize with
    /// zero values.
    bool zero_initialized;

    /// @brief If this flag is set, the pool will become synchronized pool.
    /// It means that, acquiring & releasing will become atomic operators.
    /// This is needed for multi-threaded enviroment.
    /// Destroying the pool is not synchronized by this option. So that,
    /// destroying a lock-acquiring pool is undefined behavior.
    bool synchronized;
};

struct pomelo_shared_pool_s {
    /// @brief The allocator of this pool (Not its elements)
    pomelo_allocator_t * allocator;

    /// @brief The data pool of this shared pool
    pomelo_pool_t * master_pool;

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

struct pomelo_shared_pool_options_s {
    /// @brief The allocator
    pomelo_allocator_t * allocator;

    /// @brief The master pool
    pomelo_pool_t * master_pool;

    /// @brief The buffer size of pool. Default is 16.
    /// If the number of elements in this pool decreases down to zero, it
    /// will acquire (N - buffers size) elements from the master pool.
    /// If the number of elements in this pool increases up to (2 * N), it will
    /// release (N) elements to the master pool.
    size_t buffers;
};

/* -------------------------------------------------------------------------- */
/*                               Pool APIs                                    */
/* -------------------------------------------------------------------------- */

/// @brief Set pomelo options to default
void pomelo_pool_options_init(pomelo_pool_options_t * options);

/// @brief Create new pool with options
pomelo_pool_t * pomelo_pool_create(pomelo_pool_options_t * options);

/// @brief Destroy pool
void pomelo_pool_destroy(pomelo_pool_t * pool);

/// @brief Acquire a new element from pool
/// @return Returns a new element or NULL if failed to allocate
void * pomelo_pool_acquire(pomelo_pool_t * pool);

/// @brief Release an element to pool
void pomelo_pool_release(pomelo_pool_t * pool, void * data);

/// Get the number of in-use elements
#define pomelo_pool_in_use(pool)                                               \
    ((pool)->allocated_size - (pool)->available_size)


/* -------------------------------------------------------------------------- */
/*                            Shared pool APIs                                */
/* -------------------------------------------------------------------------- */

/// @brief Initialize shared pool creating options
void pomelo_shared_pool_options_init(pomelo_shared_pool_options_t * options);

/// @brief Create new shared pool
pomelo_shared_pool_t * pomelo_shared_pool_create(
    pomelo_shared_pool_options_t * options
);

/// @brief Destroy the shared pool.
/// All holding elements will be released to the master pool
void pomelo_shared_pool_destroy(pomelo_shared_pool_t * pool);

/// @brief Acquire an element from this pool
/// @return New element
void * pomelo_shared_pool_acquire(pomelo_shared_pool_t * pool);

/// @brief Release an element to this pool
void pomelo_shared_pool_release(pomelo_shared_pool_t * pool, void * element);


/* -------------------------------------------------------------------------- */
/*                               Private APIs                                 */
/* -------------------------------------------------------------------------- */

/// @brief Acquire elements from available list of pool
/// @return Number of actually acquired elements
size_t pomelo_pool_acquire_elements(
    pomelo_pool_t * pool,
    size_t nelements,
    pomelo_pool_element_t ** elements
);

/// @brief Allocate new elements and add them to the allocated list of pool
/// @return Number of actually allocated elements
size_t pomelo_pool_allocate_elements(
    pomelo_pool_t * pool,
    size_t nelements,
    pomelo_pool_element_t ** elements
);

/// @brief Release elements
void pomelo_pool_release_elements(
    pomelo_pool_t * pool,
    size_t nelements,
    pomelo_pool_element_t ** elements
);

/// @brief Link elements to allocated list
void pomelo_pool_link_allocated(
    pomelo_pool_t * pool,
    pomelo_pool_element_t ** elements,
    size_t nelements
);

/// @brief Unlink elements from allocated list
void pomelo_pool_unlink_allocated(
    pomelo_pool_t * pool,
    pomelo_pool_element_t ** elements,
    size_t nelements
);

/// @brief Link elements to available list
void pomelo_pool_link_available(
    pomelo_pool_t * pool,
    pomelo_pool_element_t ** elements,
    size_t nelements
);

/// @brief Unlink elements from available list
void pomelo_pool_unlink_available(
    pomelo_pool_t * pool,
    pomelo_pool_element_t ** elements,
    size_t nelements
);


#ifdef __cplusplus
}
#endif
#endif // POMELO_UTILS_POOL_SRC_H

