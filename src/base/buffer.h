#ifndef POMELO_BASE_BUFFER_SRC_H
#define POMELO_BASE_BUFFER_SRC_H
#include "pomelo/statistic/statistic-buffer.h"
#include "utils/pool.h"
#include "ref.h"


/// The default buffer size of local-thread buffer context.
/// The local thread buffer context will keep some buffers to make the
/// acquisition faster.
#define POMELO_BUFFER_CONTEXT_SHARED_BUFFER_DEFAULT_SIZE 128

/// @brief Calculate the length of the buffer data for wrapping.
#define POMELO_BUFFER_CALC_WRAP_LENGTH(capacity)                               \
    ((capacity) + sizeof(pomelo_buffer_t))


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

/// @brief The non-thread safe buffer context for one and only one thread.
typedef struct pomelo_buffer_context_shared_s pomelo_buffer_context_shared_t;

/// @brief The creating options for local thread buffer context
typedef struct pomelo_buffer_context_shared_options_s
    pomelo_buffer_context_shared_options_t;

/// @brief The buffer with extra information
typedef struct pomelo_buffer_s pomelo_buffer_t;

/// @brief The buffer view
typedef struct pomelo_buffer_view_s pomelo_buffer_view_t;


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
    pomelo_statistic_buffer_t * statistic
);

/// @brief Finalize function of buffer
typedef void (*pomelo_buffer_finalize_fn)(pomelo_buffer_t * buffer);


struct pomelo_buffer_context_s {
    /// @brief The root context
    pomelo_buffer_context_root_t * root;

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

    /// @brief Whether the buffer pool is synchronized
    bool synchronized;
};


struct pomelo_buffer_context_shared_s {
    /// @brief The interface of this context
    pomelo_buffer_context_t base;

    /// @brief The allocator of this context
    pomelo_allocator_t * allocator;

    /// @brief The buffer pool of this context
    pomelo_pool_t * buffer_pool;
};


struct pomelo_buffer_context_shared_options_s {
    /// @brief The allocator
    pomelo_allocator_t * allocator;

    /// @brief The original buffer context
    pomelo_buffer_context_t * context;

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


struct pomelo_buffer_view_s {
    /// @brief The buffer of this view
    pomelo_buffer_t * buffer;

    /// @brief The offset of view
    size_t offset;

    /// @brief The length of view
    size_t length;
};


/// @brief Create new root buffer context
pomelo_buffer_context_t * pomelo_buffer_context_root_create(
    pomelo_buffer_context_root_options_t * options
);


/// @brief Create new shared buffer context
pomelo_buffer_context_t * pomelo_buffer_context_shared_create(
    pomelo_buffer_context_shared_options_t * options
);


/// @brief Destroy a buffer context
void pomelo_buffer_context_destroy(pomelo_buffer_context_t * context);


/// @brief Acquire new buffer.
/// The buffer ref counter will be set to 1.
/// @param context The buffer context to acquire buffer from
/// @return New buffer or NULL on failure
pomelo_buffer_t * pomelo_buffer_context_acquire(
    pomelo_buffer_context_t * context
);


/// @brief Get the statistic of buffer context
void pomelo_buffer_context_statistic(
    pomelo_buffer_context_t * context,
    pomelo_statistic_buffer_t * statistic
);


/* -------------------------------------------------------------------------- */
/*                               Buffer APIs                                  */
/* -------------------------------------------------------------------------- */


/// @brief Increase the reference counter of buffer
bool pomelo_buffer_ref(pomelo_buffer_t * buffer);


/// @brief Decrease the reference counter of buffer
void pomelo_buffer_unref(pomelo_buffer_t * buffer);


/// @brief Change the buffer context
void pomelo_buffer_set_context(
    pomelo_buffer_t * buffer,
    pomelo_buffer_context_t * context
);


/// @brief Get buffer object from data
#define pomelo_buffer_from_data(data) (((pomelo_buffer_t *) (data)) - 1)


/// @brief Wrap the buffer around the data.
/// This will set the ref counter of the buffer to 1.
/// @return New buffer or NULL on failure (not enough memory for buffer header)
pomelo_buffer_t * pomelo_buffer_wrap(
    void * data,
    size_t capacity,
    pomelo_buffer_finalize_fn finalize_fn
);


/* -------------------------------------------------------------------------- */
/*                                Private APIs                                */
/* -------------------------------------------------------------------------- */

/// @brief Alloc callback for buffer
int pomelo_buffer_on_alloc(
    pomelo_buffer_t * buffer,
    pomelo_buffer_context_root_t * context
);


/// @brief Init callback for buffer
int pomelo_buffer_init(
    pomelo_buffer_t * buffer,
    pomelo_buffer_context_t * context
);


/// @brief Finalize buffer before releasing to pool
void pomelo_buffer_on_finalize(pomelo_buffer_t * buffer);


/// @brief Destroy a root buffer context
void pomelo_buffer_context_root_destroy(pomelo_buffer_context_root_t * context);


/// @brief Acquire new buffer from root context
pomelo_buffer_t * pomelo_buffer_context_root_acquire(
    pomelo_buffer_context_root_t * context
);


/// @brief Release a buffer to root context
void pomelo_buffer_context_root_release(
    pomelo_buffer_context_root_t * context,
    pomelo_buffer_t * buffer
);


/// @brief Get the statistic of root buffer context
void pomelo_buffer_context_root_statistic(
    pomelo_buffer_context_root_t * context,
    pomelo_statistic_buffer_t * statistic
);


/// @brief Destroy a shared buffer context
void pomelo_buffer_context_shared_destroy(
    pomelo_buffer_context_shared_t * context
);


/// @brief Acquire new buffer from shared buffer context
pomelo_buffer_t * pomelo_buffer_context_shared_acquire(
    pomelo_buffer_context_shared_t * context
);


/// @brief Release a buffer to shared buffer context
void pomelo_buffer_context_shared_release(
    pomelo_buffer_context_shared_t * context,
    pomelo_buffer_t * buffer
);


/// @brief Get the statistic of shared buffer context
void pomelo_buffer_context_shared_statistic(
    pomelo_buffer_context_shared_t * context,
    pomelo_statistic_buffer_t * statistic
);

#ifdef __cplusplus
}
#endif
#endif // POMELO_BASE_BUFFER_SRC_H
