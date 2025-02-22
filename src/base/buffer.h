#ifndef POMELO_BASE_BUFFER_SRC_H
#define POMELO_BASE_BUFFER_SRC_H
#include "utils/pool.h"
#include "ref.h"
#include "pomelo/statistic/statistic-buffer.h"


/// The default buffer size of local-thread buffer context.
/// The local thread buffer context will keep some buffers to make the
/// acquisition faster.
#define POMELO_BUFFER_CONTEXT_SHARED_BUFFER_DEFAULT_SIZE 128


#ifdef __cplusplus
extern "C" {
#endif


/// @brief The buffer context interface
typedef struct pomelo_buffer_context_s pomelo_buffer_context_t;

/// @brief The thread-safe root buffer context
typedef struct pomelo_buffer_context_root_s pomelo_buffer_context_root_t;

/// @brief The root buffer context creating options
typedef struct pomelo_buffer_context_root_options_s
    pomelo_buffer_context_root_options_t;

#ifdef POMELO_MULTI_THREAD

/// @brief The non-thread safe buffer context for one and only one thread.
typedef struct pomelo_buffer_context_shared_s pomelo_buffer_context_shared_t;

/// @brief The creating options for local thread buffer context
typedef struct pomelo_buffer_context_shared_options_s
    pomelo_buffer_context_shared_options_t;

#endif // POMELO_MULTI_THREAD

/// @brief The buffer with extra information
typedef struct pomelo_buffer_s pomelo_buffer_t;


/// @brief The buffer acquiring function
typedef pomelo_buffer_t * (*pomelo_buffer_context_acquire_fn)(
    pomelo_buffer_context_t * context
);

/// @brief The buffer releasing function
typedef void (*pomelo_buffer_context_release_fn)(
    pomelo_buffer_context_t * context,
    pomelo_buffer_t * buffer
);

/// @brief Statistic function of buffer context
typedef void (*pomelo_buffer_context_statistic_fn)(
    pomelo_buffer_context_t * context,
    pomelo_statistic_buffer_context_t * statistic
);


struct pomelo_buffer_context_s {
    /// @brief Acquire new buffer
    pomelo_buffer_context_acquire_fn acquire;

    /// @brief Release a buffer
    pomelo_buffer_context_release_fn release;

    /// @brief Get the statistic of buffer context
    pomelo_buffer_context_statistic_fn statistic;
};


struct pomelo_buffer_context_root_s {
    /// @brief The interface of this context
    pomelo_buffer_context_t base;

    /// @brief The allocator
    pomelo_allocator_t * allocator;

    /// @brief [Synchronized] The pool of buffers
    pomelo_pool_t * buffer_pool;

    /// @brief The capacity of a buffer
    size_t buffer_capacity;
};


struct pomelo_buffer_context_root_options_s {
    /// @brief The allocator
    pomelo_allocator_t * allocator;

    /// @brief The capacity of a buffer
    size_t buffer_capacity;
};

#ifdef POMELO_MULTI_THREAD
struct pomelo_buffer_context_shared_s {
    /// @brief The interface of this context
    pomelo_buffer_context_t base;

    /// @brief The root context
    pomelo_buffer_context_root_t * root_context;

    /// @brief The allocator of this context
    pomelo_allocator_t * allocator;

    /// @brief The buffer pool of this context
    pomelo_shared_pool_t * buffer_pool;
};
#endif // POMELO_MULTI_THREAD

struct pomelo_buffer_context_shared_options_s {
    /// @brief The allocator
    pomelo_allocator_t * allocator;

    /// @brief The global buffer context
    pomelo_buffer_context_root_t * context;

    /// @brief The buffer size of local-thread buffer context
    size_t buffer_size;
};


struct pomelo_buffer_s {
    /// @brief Reference counter of buffer
    pomelo_reference_t ref;

    /// @brief Context of buffer
    pomelo_buffer_context_t * context;

    /// @brief Capacity of buffer
    size_t capacity;

    /// @brief Data pointer of buffer
    uint8_t * data;
};


/* -------------------------------------------------------------------------- */
/*                             Global buffer APIs                             */
/* -------------------------------------------------------------------------- */

/// @brief Initialize options
void pomelo_buffer_context_root_options_init(
    pomelo_buffer_context_root_options_t * options
);


/// @brief Create a data context
pomelo_buffer_context_root_t * pomelo_buffer_context_root_create(
    pomelo_buffer_context_root_options_t * options
);


/// @brief Destroy a data pool
void pomelo_buffer_context_root_destroy(pomelo_buffer_context_root_t * context);


/// @brief Acquire new buffer.
/// The buffer ref counter will be set to 1.
/// @param context The buffer context 
/// @return New buffer or NULL on failure
pomelo_buffer_t * pomelo_buffer_context_root_acquire(
    pomelo_buffer_context_root_t * context
);

/// @brief Release a buffer
void pomelo_buffer_context_root_release(
    pomelo_buffer_context_root_t * context,
    pomelo_buffer_t * buffer
);

/// @brief Get the statistic of buffer context
void pomelo_buffer_context_root_statistic(
    pomelo_buffer_context_root_t * context,
    pomelo_statistic_buffer_context_t * statistic
);


/* -------------------------------------------------------------------------- */
/*                        Shared buffer context APIs                          */
/* -------------------------------------------------------------------------- */

#ifdef POMELO_MULTI_THREAD

/// @brief Initialize the local-thread buffer context options
/// @param options The options
void pomelo_buffer_context_shared_options_init(
    pomelo_buffer_context_shared_options_t * options
);


/// @brief Create new local-thread buffer context
/// @param options The creating options
/// @return New local-thread buffer context or NULL on failure
pomelo_buffer_context_shared_t * pomelo_buffer_context_shared_create(
    pomelo_buffer_context_shared_options_t * options
);


/// @brief Destroy a local-thread buffer context
void pomelo_buffer_context_shared_destroy(
    pomelo_buffer_context_shared_t * context
);


/// @brief Acquire new buffer.
/// The buffer ref counter will be set to 1.
/// @param context The buffer context 
/// @return New buffer or NULL on failure
pomelo_buffer_t * pomelo_buffer_context_shared_acquire(
    pomelo_buffer_context_shared_t * context
);


/// @brief Release a buffer
void pomelo_buffer_context_shared_release(
    pomelo_buffer_context_shared_t * context,
    pomelo_buffer_t * buffer
);

/// @brief Get the statistic of buffer context
void pomelo_buffer_context_shared_statistic(
    pomelo_buffer_context_shared_t * context,
    pomelo_statistic_buffer_context_t * statistic
);

#endif // POMELO_MULTI_THREAD

/* -------------------------------------------------------------------------- */
/*                               Buffer APIs                                  */
/* -------------------------------------------------------------------------- */

/// @brief Increase the reference counter of buffer
/// @return 0 on success or -1 on failure
int pomelo_buffer_ref(pomelo_buffer_t * buffer);


/// @brief Decrease the reference counter of buffer
/// @return 0 on success or -1 on failure
int pomelo_buffer_unref(pomelo_buffer_t * buffer);


/// @brief Change the buffer context
void pomelo_buffer_set_context(
    pomelo_buffer_t * buffer,
    pomelo_buffer_context_t * context
);


/// @brief Get buffer object from data
#define pomelo_buffer_from_data(data) (((pomelo_buffer_t *) (data)) - 1)


/* -------------------------------------------------------------------------- */
/*                                Private APIs                                */
/* -------------------------------------------------------------------------- */

/// @brief Alloc callback for buffer
int pomelo_buffer_alloc(
    pomelo_buffer_t * buffer,
    pomelo_buffer_context_root_t * context
);


/// @brief Init callback for buffer
int pomelo_buffer_init(
    pomelo_buffer_t * buffer,
    pomelo_buffer_context_root_t * context
);


/// @brief Finalize buffer before releasing to pool
void pomelo_buffer_finalize(pomelo_buffer_t * buffer);


#ifdef __cplusplus
}
#endif
#endif // POMELO_BASE_BUFFER_SRC_H
