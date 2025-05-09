#include <assert.h>
#include <string.h>
#include <assert.h>
#include "pool.h"


/// Default buffers of shared pool
#define POMELO_SHARED_POOL_DEFAULT_BUFFERS 16 // elements

// Allocate callback is called
#define POMELO_POOL_ELEMENT_INITIALIZED     (1 << 0)

// Element has been acquired from root pool
#define POMELO_POOL_ELEMENT_ACQUIRED        (1 << 1)


#ifndef NDEBUG // Debug mode

/// The signature for all master pools
#define POMELO_POOL_SIGNATURE 0x782c82

/// The signature of all shared pools
#define POMELO_SHARED_POOL_SIGNATURE 0xaf7826

/// The signature generator for all elements of a pool
static int element_signature_generator = 0x5514ab;

/// Set pool signature
#define pomelo_pool_set_signature(pool)                                        \
    (pool)->signature = POMELO_POOL_SIGNATURE;                                 \
    (pool)->element_signature = element_signature_generator++

/// Check the pool signature in debug mode
#define pomelo_pool_check_signature(pool)                                      \
    assert((pool)->signature == POMELO_POOL_SIGNATURE)

/// Set shared pool signature
#define pomelo_shared_pool_set_signature(pool)                                 \
    (pool)->signature = POMELO_SHARED_POOL_SIGNATURE

/// Check the shared pool signature in debug mode
#define pomelo_shared_pool_check_signature(pool)                               \
    assert((pool)->signature == POMELO_SHARED_POOL_SIGNATURE)

/// Set element signature
#define pomelo_pool_set_element_signature(pool, element)                       \
    (element)->signature = (pool)->element_signature

/// Check pool element signature
#define pomelo_pool_check_element_signature(pool, element)                     \
    assert((element)->signature == (pool)->element_signature)

#else // !NDEBUG (Non-debug)

/// Set pool signature
#define pomelo_pool_set_signature(pool) (void) (pool)

/// Check the pool signature in release mode (No-op)
#define pomelo_pool_check_signature(pool) (void) (pool)

/// Set shared pool signature
#define pomelo_shared_pool_set_signature(pool) (void) (pool)

/// Check the pool signature in release mode (No-op)
#define pomelo_shared_pool_check_signature(pool) (void) (pool)

/// Set element signature
#define pomelo_pool_set_element_signature(pool, element)                       \
    (void) (pool); (void) (element)

/// Check pool element signature
#define pomelo_pool_check_element_signature(pool, element)                     \
    (void) (pool); (void) (element)

#endif // ~NDEBUG


void pomelo_pool_destroy(pomelo_pool_t * pool) {
    assert(pool != NULL);
    if (&pool->root->base == pool) {
        pomelo_pool_root_destroy((pomelo_pool_root_t *) pool);
    } else {
        pomelo_pool_shared_destroy((pomelo_pool_shared_t *) pool);
    }
}


void * pomelo_pool_acquire(pomelo_pool_t * pool, void * init_data) {
    assert(pool != NULL);
    return pool->acquire(pool, init_data);
}


void pomelo_pool_release(pomelo_pool_t * pool, void * data) {
    assert(pool != NULL);
    pool->release(pool, data);
}


pomelo_pool_t * pomelo_pool_root_create(pomelo_pool_root_options_t * options) {
    assert(options != NULL);
    if (options->element_size <= 0) return NULL; // Invalid element size

    pomelo_allocator_t * allocator = options->allocator;
    if (!allocator) {
        allocator = pomelo_allocator_default();
    }

    if (options->zero_init && (options->on_alloc || options->on_free)) {
        // Using zero_init flag with on_alloc/on_free is not allowed
        assert(false && "zero_init + on_alloc/on_free is not allowed");
        return NULL;
    }

    // Allocate memory for pool
    pomelo_pool_root_t * pool =
        pomelo_allocator_malloc_t(allocator, pomelo_pool_root_t);
    if (!pool) return NULL;
    memset(pool, 0, sizeof(pomelo_pool_root_t));
    pomelo_pool_set_signature(pool);

    pool->allocator = allocator;
    pool->element_size = options->element_size;
    pool->available_max = options->available_max;
    pool->allocated_max = options->allocated_max;
    pool->on_alloc = options->on_alloc;
    pool->on_free = options->on_free;
    pool->on_cleanup = options->on_cleanup;
    pool->on_init = options->on_init;
    pool->alloc_data = options->alloc_data;
    pool->zero_init = options->zero_init;

    if (options->synchronized) {
        // This pool is synchronized.
        pool->mutex = pomelo_mutex_create(allocator);
        if (!pool->mutex) {
            pomelo_pool_root_destroy(pool);
            return NULL;
        }
    }

    // Initialize base
    pomelo_pool_t * base = &pool->base;
    base->root = pool; // Itself
    base->acquire = (pomelo_pool_acquire_fn) pomelo_pool_root_acquire;
    base->release = (pomelo_pool_release_fn) pomelo_pool_root_release;

    return base;
}


void pomelo_pool_root_destroy(pomelo_pool_root_t * pool) {
    assert(pool != NULL);
    pomelo_pool_check_signature(pool);

    pomelo_allocator_t * allocator = pool->allocator;
    pomelo_pool_element_t * element = pool->allocated_elements;
    pomelo_pool_element_t * current;
    while (element) {
        current = element;
        element = element->allocated_next;
        uint32_t flags = current->flags;
        if (pool->on_cleanup) {
            if (flags & POMELO_POOL_ELEMENT_ACQUIRED) {
                pool->on_cleanup(current + 1);
            }
        }

        if (pool->on_free) {
            if (flags & POMELO_POOL_ELEMENT_INITIALIZED) {
                pool->on_free(current + 1);
            }
        }
        pomelo_allocator_free(allocator, current);
    }

    pool->available_elements = NULL;
    pool->allocated_elements = NULL;

    // Release the mutex
    if (pool->mutex) {
        pomelo_mutex_destroy(pool->mutex);
        pool->mutex = NULL;
    }

    pomelo_allocator_free(allocator, pool);
}


void * pomelo_pool_root_acquire(pomelo_pool_root_t * pool, void * init_data) {
    assert(pool != NULL);
    pomelo_pool_check_signature(pool);

    // Check if init_data is provided when on_init is not set accidentally
    assert(pool->on_init || !init_data);

    // Try to get from available list
    pomelo_pool_element_t * element = NULL;
    pomelo_pool_acquire_elements(pool, 1, &element);
    
    void * data = NULL;
    if (!element) {
        // No more element in pool, try to allocate new one
        pomelo_pool_allocate_elements(pool, 1, &element);
        if (!element) {
            // Failed to allocate new element
            return NULL;
        }

        element->flags = 0; // Reset flag

        data = element + 1;
        if (pool->on_alloc) {
            int ret = pool->on_alloc(data, pool->alloc_data);
            if (ret < 0) {
                // Call the on_free (if any)
                if (pool->on_free) {
                    pool->on_free(data);
                }

                // Allocating callback returns an error
                pomelo_pool_release_elements(pool, 1, &element);
                return NULL;
            }
        }

        // Allocate callback is called
        element->flags |= POMELO_POOL_ELEMENT_INITIALIZED;
    } else {
        data = element + 1;
    }

    if (pool->zero_init) {
        memset(data, 0, pool->element_size);
    }

    if (pool->on_init) {
        int ret = pool->on_init(data, init_data);
        if (ret < 0) {
            // Acquiring callback returns an error
            // Call the on_cleanup (if any)
            if (pool->on_cleanup) {
                pool->on_cleanup(data);
            }

            pomelo_pool_release_elements(pool, 1, &element);
            return NULL;
        }
    }

    // Update element values
    element->available_next = NULL;
    element->flags |= POMELO_POOL_ELEMENT_ACQUIRED;

    // Failed to allocate new element
    return data;
}


void pomelo_pool_root_release(pomelo_pool_root_t * pool, void * data) {
    assert(pool != NULL);
    assert(data != NULL);
    pomelo_pool_check_signature(pool);

    pomelo_pool_element_t * element = ((pomelo_pool_element_t *) data) - 1;
    pomelo_pool_check_element_signature(pool, element);

    if (!(element->flags & POMELO_POOL_ELEMENT_ACQUIRED)) {
        return; // The element's already released
    }

    if (pool->on_cleanup) {
        pool->on_cleanup(data);
    }

    // Clear acquired flag
    element->flags &= ~POMELO_POOL_ELEMENT_ACQUIRED;

    pomelo_pool_release_elements(pool, 1, &element);
}


size_t pomelo_pool_acquire_elements(
    pomelo_pool_root_t * pool,
    size_t nelements,
    pomelo_pool_element_t ** elements
) {
    assert(pool != NULL);
    assert(elements != NULL);

    size_t acquired_counter = 0;

    pomelo_mutex_t * mutex = pool->mutex;
    POMELO_BEGIN_CRITICAL_SECTION(mutex);
    /**  Begin critical section  **/

    pomelo_pool_element_t * element = pool->available_elements;
    while (acquired_counter < nelements && element) {
        // Remove element from available list
        elements[acquired_counter++] = element;
        element = element->available_next;
    }

    // Unlink acquired elements from available list
    pomelo_pool_unlink_available(pool, elements, acquired_counter);

    /**  End critical section **/
    POMELO_END_CRITICAL_SECTION(mutex);

    return acquired_counter;
}


size_t pomelo_pool_allocate_elements(
    pomelo_pool_root_t * pool,
    size_t nelements,
    pomelo_pool_element_t ** elements
) {
    assert(pool != NULL);
    assert(elements != NULL);

    pomelo_mutex_t * mutex = pool->mutex;
    POMELO_BEGIN_CRITICAL_SECTION(mutex);

    size_t nelements_cap = nelements;
    if (
        pool->allocated_max > 0 &&
        pool->allocated_size + nelements > pool->allocated_max
    ) {
        nelements_cap = pool->allocated_max - pool->allocated_size;
    }

    size_t element_size = sizeof(pomelo_pool_element_t) + pool->element_size;
    size_t nallocated = 0;
    while (nallocated < nelements_cap) {
        pomelo_pool_element_t * element =
            pomelo_allocator_malloc(pool->allocator, element_size);
        if (!element) break; // Failed to allocate new element
        memset(element, 0, element_size); // Set new element with zero
        pomelo_pool_set_element_signature(pool, element);
        elements[nallocated++] = element;
    }

    // Link allocated elements
    pomelo_pool_link_allocated(pool, elements, nallocated);

    POMELO_END_CRITICAL_SECTION(mutex);

    return nallocated;
}


void pomelo_pool_release_elements(
    pomelo_pool_root_t * pool,
    size_t nelements,
    pomelo_pool_element_t ** elements
) {
    assert(pool != NULL);
    assert(elements != NULL);

    pomelo_mutex_t * mutex = pool->mutex;
    POMELO_BEGIN_CRITICAL_SECTION(mutex);
    /**  Begin critical section  **/

    size_t nfree = 0;
    size_t nrelease = nelements;
    if (
        pool->available_max > 0 &&
        pool->available_size + nelements > pool->available_max
    ) {
        nrelease = pool->available_max - pool->available_size;
        nfree = nelements - nrelease;
    }

    // Add released elements to available list and remove freed elements from
    // allocated list
    pomelo_pool_link_available(pool, elements, nrelease);
    pomelo_pool_unlink_allocated(pool, elements + nrelease, nfree);

    /**  End critical section **/
    POMELO_END_CRITICAL_SECTION(mutex);

    // Free elements
    for (size_t i = 0; i < nfree; i++) {
        pomelo_allocator_free(pool->allocator, elements[nrelease + i]);
    }
}


/* -------------------------------------------------------------------------- */
/*                            Shared pool APIs                                */
/* -------------------------------------------------------------------------- */

pomelo_pool_t * pomelo_pool_shared_create(
    pomelo_pool_shared_options_t * options
) {
    assert(options != NULL);
    if (!options->origin_pool) return NULL; // No origin pool is provided
    pomelo_pool_root_t * root = options->origin_pool->root;
    assert(root != NULL); // Root pool is required
    if (!root->mutex) {
        assert(false && "Root pool must be synchronized");
        return NULL; // Root pool is not synchronized
    }

    size_t buffers = options->buffers;
    if (buffers == 0) {
        buffers = POMELO_SHARED_POOL_DEFAULT_BUFFERS;
    }

    pomelo_allocator_t * allocator = options->allocator;
    if (!allocator) {
        allocator = pomelo_allocator_default();
    }

    pomelo_pool_shared_t * pool =
        pomelo_allocator_malloc_t(allocator, pomelo_pool_shared_t);
    if (!pool) return NULL; // Failed to allocate new pool

    memset(pool, 0, sizeof(pomelo_pool_shared_t));
    pomelo_shared_pool_set_signature(pool);

    pool->allocator = allocator;
    pool->buffers = buffers;
    pool->nelements = buffers * 2;

    pool->elements = pomelo_allocator_malloc(
        allocator,
        pool->nelements * sizeof(pomelo_pool_element_t *)
    );
    if (!pool->elements) {
        pomelo_pool_shared_destroy(pool);
        return NULL;
    }

    // Initialize base
    pomelo_pool_t * base = &pool->base;
    base->root = root;
    base->acquire = (pomelo_pool_acquire_fn) pomelo_shared_pool_acquire;
    base->release = (pomelo_pool_release_fn) pomelo_pool_shared_release;

    return base;
}


void pomelo_pool_shared_destroy(pomelo_pool_shared_t * pool) {
    assert(pool != NULL);
    pomelo_shared_pool_check_signature(pool);

    pomelo_allocator_t * allocator = pool->allocator;

    if (pool->index > 0) {
        // Release all holding elements
        pomelo_pool_release_elements(
            pool->base.root,
            pool->index,
            pool->elements
        );
    }

    if (pool->elements) {
        pomelo_allocator_free(allocator, pool->elements);
        pool->elements = NULL;
    }

    // Free the pool itself
    pomelo_allocator_free(allocator, pool);
}


void * pomelo_shared_pool_acquire(
    pomelo_pool_shared_t * pool,
    void * init_data
) {
    assert(pool != NULL);

    pomelo_shared_pool_check_signature(pool);
    pomelo_pool_root_t * root = pool->base.root;

    if (pool->index == 0) {
        pool->index += pomelo_pool_acquire_elements(
            root,
            pool->buffers,
            pool->elements
        );
    }

    if (pool->index == 0) {
        // No more element to acquire, request allocating new elements
        pool->index += pomelo_pool_allocate_elements(
            root,
            pool->buffers,
            pool->elements
        );
    }

    if (pool->index == 0) {
        // Cannot allocate more elements
        return NULL;
    }

    // Get element from pool
    pomelo_pool_element_t * element = pool->elements[--pool->index];
    void * data = element + 1;

    // Initialize (if it has not been done before yet)
    if (!(element->flags & POMELO_POOL_ELEMENT_INITIALIZED)) {
        if (root->on_alloc) {
            int ret = root->on_alloc(data, root->alloc_data);
            if (ret < 0) {
                // Call the on_free (if any)
                if (root->on_free) {
                    root->on_free(data);
                }

                // Failed to acquire, return to pool
                pool->elements[pool->index++] = data;
                return NULL;
            }
        }

        element->flags |= POMELO_POOL_ELEMENT_INITIALIZED;
    }

    // Set the shared acquired flag
    element->flags |= POMELO_POOL_ELEMENT_ACQUIRED;

    // Call the callbacks
    if (root->zero_init) {
        memset(data, 0, root->element_size);
    }

    if (root->on_init) {
        // Call the callback
        int ret = root->on_init(data, init_data);
        if (ret < 0) {
            // Failed to acquire, return to pool
            // Call the on_cleanup (if any)
            if (root->on_cleanup) {
                root->on_cleanup(data);
            }

            pool->elements[pool->index++] = data;
            return NULL;
        }
    }

    return data;
}


void pomelo_pool_shared_release(pomelo_pool_shared_t * pool, void * data) {
    assert(pool != NULL);
    assert(data != NULL);

    pomelo_shared_pool_check_signature(pool);

    pomelo_pool_root_t * root = pool->base.root;
    pomelo_pool_element_t * element = ((pomelo_pool_element_t *) data) - 1;
    pomelo_pool_check_element_signature(root, element);

    // Element acquired from root pool could be released to shared pool as well
    assert(element->flags & POMELO_POOL_ELEMENT_ACQUIRED);
    if (!(element->flags & POMELO_POOL_ELEMENT_ACQUIRED)) {
        return; // The element has been released
    }

    if (pool->index == pool->nelements) {
        // Release half of elements
        pomelo_pool_release_elements(
            root,
            pool->buffers,
            pool->elements + pool->buffers
        );

        pool->index -= pool->buffers;
    }

    if (root->on_cleanup) {
        root->on_cleanup(data);
    }

    // Remove the shared acquired flag
    element->flags &= ~POMELO_POOL_ELEMENT_ACQUIRED;

    // Push the element to the back of array
    pool->elements[pool->index++] = element;
}


void pomelo_pool_link_allocated(
    pomelo_pool_root_t * pool,
    pomelo_pool_element_t ** elements,
    size_t nelements
) {
    assert(pool != NULL);
    assert(elements != NULL);
    if (nelements == 0) {
        return;
    }

    // Link elements in array together
    size_t last = nelements - 1;
    for (size_t i = 1; i < last; i++) {
        pomelo_pool_element_t * element = elements[i];
        element->allocated_next = elements[i + 1];
        element->allocated_prev = elements[i - 1];
    }
    if (nelements > 1) {
        elements[0]->allocated_next = elements[1];
        elements[last]->allocated_prev = elements[last - 1];
    }

    // Link new elements with list
    pomelo_pool_element_t * head = pool->allocated_elements;
    elements[last]->allocated_next = head;
    elements[0]->allocated_prev = NULL;

    // Update head of list
    if (head) {
        head->allocated_prev = elements[last];
    }
    pool->allocated_elements = elements[0];

    pool->allocated_size += nelements;
}


void pomelo_pool_unlink_allocated(
    pomelo_pool_root_t * pool,
    pomelo_pool_element_t ** elements,
    size_t nelements
) {
    assert(pool != NULL);
    assert(elements != NULL);
    assert(pool->allocated_size >= nelements);
    
    for (size_t i = 0; i < nelements; i++) {
        pomelo_pool_element_t * element = elements[i];
        pomelo_pool_element_t * prev = element->allocated_prev;
        pomelo_pool_element_t * next = element->allocated_next;
        if (prev) {
            prev->allocated_next = next;
        }
        if (next) {
            next->allocated_prev = prev;
        }

        if (element == pool->allocated_elements) {
            // Element is head
            pool->allocated_elements = next;
        }
    }

    pool->allocated_size -= nelements;
}


void pomelo_pool_link_available(
    pomelo_pool_root_t * pool,
    pomelo_pool_element_t ** elements,
    size_t nelements
) {
    assert(pool != NULL);
    assert(elements != NULL);
    if (nelements == 0) {
        return;
    }

    // Link elements in array together
    size_t last = nelements - 1;
    for (size_t i = 1; i < last; i++) {
        pomelo_pool_element_t * element = elements[i];
        element->available_next = elements[i + 1];
        element->available_prev = elements[i - 1];
    }
    if (nelements > 1) {
        elements[0]->available_next = elements[1];
        elements[last]->available_prev = elements[last - 1];
    }

    // Link new elements with list
    pomelo_pool_element_t * head = pool->available_elements;
    elements[last]->available_next = head;
    elements[0]->available_prev = NULL;
    
    // Update head
    if (head) {
        head->available_prev = elements[last];
    }
    pool->available_elements = elements[0];

    pool->available_size += nelements;
}


void pomelo_pool_unlink_available(
    pomelo_pool_root_t * pool,
    pomelo_pool_element_t ** elements,
    size_t nelements
) {
    assert(pool != NULL);
    assert(elements != NULL);
    assert(pool->available_size >= nelements);
    if (nelements == 0) {
        return;
    }

    for (size_t i = 0; i < nelements; i++) {
        pomelo_pool_element_t * element = elements[i];
        pomelo_pool_element_t * prev = element->available_prev;
        pomelo_pool_element_t * next = element->available_next;
        if (prev) {
            prev->available_next = next;
        }
        if (next) {
            next->available_prev = prev;
        }

        if (element == pool->available_elements) {
            // Element is head
            pool->available_elements = next;
        }
    }
    
    pool->available_size -= nelements;
}
