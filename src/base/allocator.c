#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "pomelo/allocator.h"
#ifdef POMELO_MULTI_THREAD
#include "utils/atomic.h"
#endif // POMELO_MULTI_THREAD

#ifndef NDEBUG // Debug mode

/// The signature of all allocators
#define POMELO_ALLOCATOR_SIGNATURE 0x481cfa

/// The signature generator for all elements of allocator
static int element_signature_generator = 0x76a51f;

/// Check the signature of allocator in debug mode
#define pomelo_allocator_check_signature(allocator) \
    assert(allocator->signature == POMELO_ALLOCATOR_SIGNATURE)

#else // !NDEBUG

/// Check the signature of allocator in release mode (No op)
#define pomelo_allocator_check_signature(allocator) (void) allocator

#endif // ~NDEBUG


#ifdef POMELO_MULTI_THREAD
typedef pomelo_atomic_uint64_t pomelo_allocator_allocated_bytes_t;

#define pomelo_allocator_allocated_bytes_add(bytes, value) \
    pomelo_atomic_uint64_fetch_add(&(bytes), value)

#define pomelo_allocator_allocated_bytes_sub(bytes, value) \
    pomelo_atomic_uint64_fetch_sub(&(bytes), value)

#define pomelo_allocator_allocated_bytes_get(bytes) \
    pomelo_atomic_uint64_load(&(bytes))

#define pomelo_allocator_allocated_bytes_set(bytes, value) \
    pomelo_atomic_uint64_store(&(bytes), value)


#else // !POMELO_MULTI_THREAD
typedef uint64_t pomelo_allocator_allocated_bytes_t;

#define pomelo_allocator_allocated_bytes_add(bytes, value) (bytes) += (value)

#define pomelo_allocator_allocated_bytes_sub(bytes, value) (bytes) -= (value)

#define pomelo_allocator_allocated_bytes_get(bytes) (bytes)

#define pomelo_allocator_allocated_bytes_set(bytes, value) (bytes) = (value)


#endif // POMELO_MULTI_THREAD


struct pomelo_allocator_s {
    /// @brief The allocator context
    void * context;

    /// @brief The allocator malloc
    pomelo_alloc_callback malloc;

    /// @brief The allocator free
    pomelo_free_callback free;

    /// @brief Failure callback
    pomelo_alloc_failure_callback failure_callback;

    /// @brief Total in-use bytes of data
    pomelo_allocator_allocated_bytes_t allocated_bytes;

#ifndef NDEBUG
    /// @brief The signature of allocator
    int signature;

    /// @brief The signature of all memory blocks created by this allocator
    int element_signature;
#endif

};

struct pomelo_allocator_header_s;

/// @brief The header for allocator
typedef struct pomelo_allocator_header_s pomelo_allocator_header_t;


struct pomelo_allocator_header_s {
    /// @brief The size of memory
    size_t size;

#ifndef NDEBUG
    /// @brief The signature of memory block
    int signature;
#endif
};


/// The default allocator
static pomelo_allocator_t * pomelo_default_allocator = NULL;


/// @brief Initialize new allocator
static void pomelo_allocator_init(pomelo_allocator_t * allocator) {
    assert(allocator != NULL);
    memset(allocator, 0, sizeof(pomelo_allocator_t));
#ifndef NDEBUG
    allocator->element_signature = element_signature_generator++;
    allocator->signature = POMELO_ALLOCATOR_SIGNATURE;
#endif

    pomelo_allocator_allocated_bytes_set(allocator->allocated_bytes, 0);
}


pomelo_allocator_t * pomelo_allocator_default(void) {
    if (!pomelo_default_allocator) {
        pomelo_default_allocator = malloc(sizeof(pomelo_allocator_t));
        if (!pomelo_default_allocator) {
            return NULL;
        }
        pomelo_allocator_init(pomelo_default_allocator);
    }

    return pomelo_default_allocator;
}


void * pomelo_allocator_malloc(pomelo_allocator_t * allocator, size_t size) {
    assert(allocator != NULL);
    pomelo_allocator_check_signature(allocator);

    if (size == 0) {
        return NULL;
    }

    void * data = NULL;
    if (allocator == pomelo_default_allocator) {
        data = malloc(size + sizeof(pomelo_allocator_header_t));
    } else {
        data = allocator->malloc(
            allocator->context,
            size + sizeof(pomelo_allocator_header_t)
        );
    }

    if (!data) {
        // Failed to allocate
        if (allocator->failure_callback) {
            allocator->failure_callback(allocator->context, size);
        }
        return NULL;
    }

    // Update the header data
    pomelo_allocator_header_t * header = data;
    header->size = size;

#ifndef NDEBUG // Debug mode
    header->signature = allocator->element_signature;
    memset(header + 1, 0xcc, size); // Set dummy value for better debugging
#endif

    // For statistic
    pomelo_allocator_allocated_bytes_add(
        allocator->allocated_bytes,
        (uint64_t) size
    );

    return header + 1; // Skip the header for payload
}


void pomelo_allocator_free(pomelo_allocator_t * allocator, void * mem) {
    assert(allocator != NULL);
    assert(mem != NULL);
    pomelo_allocator_check_signature(allocator);

    // Point backward to find the header
    pomelo_allocator_header_t * header =
        ((pomelo_allocator_header_t *) mem) - 1;

#ifndef NDEBUG // Debug mode
    assert(header->signature == allocator->element_signature);
#endif

    // For statistic
    pomelo_allocator_allocated_bytes_sub(
        allocator->allocated_bytes,
        (uint64_t) header->size
    );


    if (allocator == pomelo_default_allocator) {
        free(header);
    } else {
        allocator->free(allocator->context, header);
    }
}


uint64_t pomelo_allocator_allocated_bytes(pomelo_allocator_t * allocator) {
    assert(allocator != NULL);
    return pomelo_allocator_allocated_bytes_get(allocator->allocated_bytes);
}


pomelo_allocator_t * pomelo_allocator_create(
    void * context,
    pomelo_alloc_callback alloc_callback,
    pomelo_free_callback free_callback
) {
    assert(alloc_callback != NULL);
    assert(free_callback != NULL);

    pomelo_allocator_t * allocator =
        alloc_callback(context, sizeof(pomelo_allocator_t));
    if (!allocator) {
        return NULL;
    }

    pomelo_allocator_init(allocator);
    allocator->malloc = alloc_callback;
    allocator->free = free_callback;
    allocator->context = context;

    return allocator;
}


/// @brief Destroy an allocator
void pomelo_allocator_destroy(pomelo_allocator_t * allocator) {
    assert(allocator != NULL);

    pomelo_allocator_check_signature(allocator);

    if (allocator == pomelo_default_allocator) {
        free(allocator);
        return;
    }

    pomelo_free_callback free_fn = allocator->free;
    void * context = allocator->context;

    free_fn(context, allocator);
}


void pomelo_allocator_set_failure_callback(
    pomelo_allocator_t * allocator,
    pomelo_alloc_failure_callback callback
) {
    assert(allocator != NULL);
    allocator->failure_callback = callback;
}
