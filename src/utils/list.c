#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include "list.h"
#include "macro.h"


#ifndef NDEBUG // Debug mode

#define POMELO_LIST_SIGNATURE 0x27e241

// Signature counter for debugging
static int node_signature_counter = 0;

#define pomelo_list_check_signature(list)                                      \
    assert(list->signature == POMELO_LIST_SIGNATURE)
#else // Release mode
#define pomelo_list_check_signature(list) (void) list
#endif

/* -------------------------------------------------------------------------- */
/*                               Private APIs                                 */
/* -------------------------------------------------------------------------- */

/// @brief Initialize node
static int pomelo_list_node_init(
    pomelo_list_node_t * node,
    pomelo_list_t * list
) {
    assert(node != NULL);
    assert(list != NULL);

    node->next = NULL;
    node->prev = NULL;

#ifndef NDEBUG
    node->signature = list->node_signature;
#else
    (void) list;
#endif

    return 0;
}


/// @brief Append new node to back of the list
static pomelo_list_node_t * pomelo_list_append_back(pomelo_list_t * list) {
    assert(list != NULL);
    pomelo_list_check_signature(list);

    pomelo_list_node_t * node = pomelo_pool_acquire(list->node_pool);
    if (!node) {
        return NULL;
    }

    if (list->size == 0) {
        // Empty
        list->front = node;
        list->back = node;
        node->next = NULL;
        node->prev = NULL;
    } else {
        node->prev = list->back;
        node->next = NULL;
        list->back->next = node;
        list->back = node;
    }

    list->size++;
    list->mod_count++;
    return node;
}


/// @brief Append new node to front of the list
static pomelo_list_node_t * pomelo_list_append_front(pomelo_list_t * list) {
    pomelo_list_node_t * node = pomelo_pool_acquire(list->node_pool);
    if (!node) {
        return NULL;
    }

    if (list->size == 0) {
        // Empty
        list->front = node;
        list->back = node;
        node->next = NULL;
        node->prev = NULL;
    } else {
        node->next = list->front;
        node->prev = NULL;
        list->front->prev = node;
        list->front = node;
    }

    list->size++;
    list->mod_count++;
    return node;
}


/// @brief Remove a node
static void pomelo_list_remove_node(
    pomelo_list_t * list,
    pomelo_list_node_t * node
) {
    if (node == list->front) {
        // Node is the front of list
        list->front = node->next;
        if (list->front != NULL) {
            // Non-empty list
            list->front->prev = NULL;
        } else {
            // Emtpy list
            list->back = NULL;
        }
        node->next = NULL;
    } else if (node == list->back) {
        list->back = node->prev;
        if (list->back) {
            // Non-empty list
            list->back->next = NULL;
        } else {
            // Emtpy list
            list->front = NULL;
        }
        node->prev = NULL;
    } else {
        if (!node->next && !node->prev) {
            // The node has already been removed
            return;
        }

        pomelo_list_node_t * prev = node->prev;
        pomelo_list_node_t * next = node->next;
        prev->next = next;
        next->prev = prev;
        node->next = NULL;
        node->prev = NULL;
    }

    list->size--;
    list->mod_count++;

    // Release node to pool
    pomelo_pool_release(list->node_pool, node);
}

/* -------------------------------------------------------------------------- */
/*                                Public APIs                                 */
/* -------------------------------------------------------------------------- */

void pomelo_list_options_init(pomelo_list_options_t * options) {
    assert(options);
    memset(options, 0, sizeof(pomelo_list_options_t));
}


pomelo_list_t * pomelo_list_create(pomelo_list_options_t * options) {
    assert(options != NULL);
    if (options->element_size == 0) {
        return NULL;
    }

    pomelo_allocator_t * allocator = options->allocator;
    if (!allocator) {
        allocator = pomelo_allocator_default();
    }

    // Create list base
    pomelo_list_t * list = pomelo_allocator_malloc_t(allocator, pomelo_list_t);
    if (!list) {
        return NULL;
    }

    memset(list, 0, sizeof(pomelo_list_t));
    list->allocator = allocator;
    list->element_size = options->element_size;

#ifndef NDEBUG
    // Set signature for list
    list->signature = POMELO_LIST_SIGNATURE;
    list->node_signature = node_signature_counter++;
#endif

    // Create node pool
    pomelo_pool_options_t pool_options;
    pomelo_pool_options_init(&pool_options);
    pool_options.allocator = allocator;
    pool_options.callback_context = list;
    pool_options.allocate_callback = (pomelo_pool_callback)
        pomelo_list_node_init;

    // element size = (header size) + (element size)
    pool_options.element_size = sizeof(pomelo_list_node_t) + list->element_size;

    list->node_pool = pomelo_pool_create(&pool_options);
    if (!list->node_pool) {
        pomelo_list_destroy(list);
        return NULL;
    }

    if (options->synchronized) {
        list->mutex = pomelo_mutex_create(allocator);
        if (!list->mutex) {
            pomelo_list_destroy(list);
            return NULL;
        }
    }

    return list;
}


void pomelo_list_destroy(pomelo_list_t * list) {
    assert(list != NULL);
    pomelo_list_check_signature(list);

    // Destroy all nodes
    if (list->node_pool) {
        pomelo_pool_destroy(list->node_pool);
        list->node_pool = NULL;
    }

    // Destroy mutex
    if (list->mutex) {
        pomelo_mutex_destroy(list->mutex);
        list->mutex = NULL;
    }

    // Free the list itself
    pomelo_allocator_free(list->allocator, list);
}


int pomelo_list_resize(pomelo_list_t * list, size_t size) {
    assert(list != NULL);
    bool success = true;

    pomelo_mutex_t * mutex = list->mutex;
    if (mutex) {
        pomelo_mutex_lock(mutex);
    }
    /* Begin critical section */

    while (list->size < size) {
        // Need more
        pomelo_list_node_t * node = pomelo_list_append_back(list);
        if (!node) {
            success = false;
            break;
        }

        // Zero-initialize the element
        memset(pomelo_list_pelement(node), 0, list->element_size);
    }

    while (list->size > size) {
        pomelo_list_remove_node(list, list->back);
    }

    /* End critical section */
    if (mutex) {
        pomelo_mutex_unlock(mutex);
    }
    return success ? 0 : -1;
}


pomelo_list_node_t * pomelo_list_push_front_p(
    pomelo_list_t * list,
    void * p_element
) {
    assert(list != NULL);
    assert(p_element != NULL);
    pomelo_list_check_signature(list);
    
    pomelo_mutex_t * mutex = list->mutex;
    if (mutex) {
        pomelo_mutex_lock(mutex);
    }
    /* Begin critical section */

    pomelo_list_node_t * node = pomelo_list_append_front(list);
    if (node) {
        memcpy(pomelo_list_pelement(node), p_element, list->element_size);
    }

    /* End critical section */
    if (mutex) {
        pomelo_mutex_unlock(mutex);
    }
    return node;
}


int pomelo_list_pop_front(pomelo_list_t * list, void * data) {
    assert(list != NULL);
    assert(data != NULL);
    pomelo_list_check_signature(list);
    int result = 0;
    
    pomelo_mutex_t * mutex = list->mutex;
    if (mutex) {
        pomelo_mutex_lock(mutex);
    }
    /* Begin critical section */
    
    if (list->front) {
        if (data) {
            memcpy(data, pomelo_list_pelement(list->front), list->element_size);
        }
        pomelo_list_remove_node(list, list->front);
    } else {
        // No front element
        result = -1;
    }
    
    /* End critical section */
    if (mutex) {
        pomelo_mutex_unlock(mutex);
    }

    return result;
}


pomelo_list_node_t * pomelo_list_push_back_p(
    pomelo_list_t * list,
    void * p_element
) {
    assert(list != NULL);
    assert(p_element != NULL);
    pomelo_list_check_signature(list);
    
    pomelo_mutex_t * mutex = list->mutex;
    if (mutex) {
        pomelo_mutex_lock(mutex);
    }
    /* Begin critical section */

    pomelo_list_node_t * node = pomelo_list_append_back(list);
    if (node) {
        memcpy(pomelo_list_pelement(node), p_element, list->element_size);
    }

    /* End critical section */
    if (mutex) {
        pomelo_mutex_unlock(mutex);
    }
    return node;
}


int pomelo_list_pop_back(pomelo_list_t * list, void * data) {
    assert(list != NULL);
    assert(data != NULL);
    pomelo_list_check_signature(list);
    int result = 0;
    
    pomelo_mutex_t * mutex = list->mutex;
    if (mutex) {
        pomelo_mutex_lock(mutex);
    }
    /* Begin critical section */

    if (list->back) {
        if (data) {
            memcpy(data, pomelo_list_pelement(list->back), list->element_size);
        }

        pomelo_list_remove_node(list, list->back);
    } else {
        // No back element
        result = -1;
    }

    /* End critical section */
    if (mutex) {
        pomelo_mutex_unlock(mutex);
    }
    return result;
}


void pomelo_list_remove(pomelo_list_t * list, pomelo_list_node_t * node) {
    assert(list != NULL);
    assert(node != NULL);
    pomelo_list_check_signature(list);

#ifndef NDEBUG
    assert(list->node_signature == node->signature);
#endif
    pomelo_mutex_t * mutex = list->mutex;
    if (mutex) {
        pomelo_mutex_lock(mutex);
    }
    /* Begin critical section */

    pomelo_list_remove_node(list, node);

    /* End critical section */
    if (mutex) {
        pomelo_mutex_unlock(mutex);
    }
}


void pomelo_list_clear(pomelo_list_t * list) {
    assert(list != NULL);
    pomelo_list_check_signature(list);

    pomelo_mutex_t * mutex = list->mutex;
    if (mutex) {
        pomelo_mutex_lock(mutex);
    }
    /* Begin critical section */

    pomelo_list_node_t * node = list->front;
    while (node) {
        pomelo_pool_release(list->node_pool, node);
        node = node->next;
    }

    list->size = 0;
    list->front = NULL;
    list->back = NULL;
    list->mod_count++;

    /* End critical section */
    if (mutex) {
        pomelo_mutex_unlock(mutex);
    }
}


bool pomelo_list_is_empty(pomelo_list_t * list) {
    assert(list != NULL);
    bool result = false;
    pomelo_mutex_t * mutex = list->mutex;
    if (mutex) {
        pomelo_mutex_lock(mutex);
    }
    /* Begin critical section */

    result = (list->front == NULL);

    /* End critical section */
    if (mutex) {
        pomelo_mutex_unlock(mutex);
    }

    return result;
}


void pomelo_list_iterate(pomelo_list_t * list, pomelo_list_iterator_t * it) {
    assert(list != NULL);
    assert(it != NULL);

    pomelo_mutex_t * mutex = list->mutex;
    if (mutex) {
        pomelo_mutex_lock(mutex);
    }
    /* Begin critical section */

    it->list = list;
    it->current = NULL;
    it->mod_count = list->mod_count;

    if (list->size == 0) {
        // This is empty
        it->next = NULL;
    } else {
        it->next = list->front;
    }

    /* End critical section */
    if (mutex) {
        pomelo_mutex_unlock(mutex);
    }
}


void * pomelo_list_iterator_next(
    pomelo_list_iterator_t * it,
    void * p_element
) {
    assert(it != NULL);
    if (!it->next) {
        return NULL; // No more element
    }

    pomelo_mutex_t * mutex = it->list->mutex;
    if (mutex) {
        pomelo_mutex_lock(mutex);
    }
    /* Begin critical section */

    assert(it->mod_count == it->list->mod_count);
    if (it->mod_count != it->list->mod_count) {
        // Mod count has changed
        if (mutex) {
            pomelo_mutex_unlock(mutex);
        }
        return NULL;
    }

    it->current = it->next;
    it->next = it->next->next;

    void * element = pomelo_list_pelement(it->current);
    if (p_element) {
        // Clone data
        memcpy(p_element, element, it->list->element_size);
    }

    /* End critical section */
    if (mutex) {
        pomelo_mutex_unlock(mutex);
    }

    return element;
}


void pomelo_list_iterator_remove(pomelo_list_iterator_t * it) {
    assert(it != NULL);

    if (!it->current) {
        return;
    }

    pomelo_mutex_t * mutex = it->list->mutex;
    if (mutex) {
        pomelo_mutex_lock(mutex);
    }
    /* Begin critical section */

    pomelo_list_node_t * node = it->current;
    it->current = node->prev;

    // Remove the current node
    pomelo_list_remove_node(it->list, node);

    /* End critical section */
    if (mutex) {
        pomelo_mutex_unlock(mutex);
    }
}


/* -------------------------------------------------------------------------- */
/*                               Unrolled list                                */
/* -------------------------------------------------------------------------- */

/// @brief Find the bucket by index
static uint8_t * pomelo_unrolled_list_find_bucket(
    pomelo_unrolled_list_t * list,
    size_t element_index
) {
    assert(list != NULL);

    size_t i;
    size_t bucket_index = element_index / list->bucket_elements;

    pomelo_list_node_t * node = NULL;

    if (bucket_index < list->size / 2) {
        // At the left half
        node = list->nodes->front;
        i = 0;

        while (++i < bucket_index) {
            node = node->next;
            assert(node != NULL);
        }
    } else {
        // At the right half
        node = list->nodes->back;
        i = list->size;

        while (--i > bucket_index) {
            node = node->prev;
            assert(node != NULL);
        }
    }

    return pomelo_list_pelement(node);
}



void pomelo_unrolled_list_options_init(
    pomelo_unrolled_list_options_t * options
) {
    assert(options != NULL);
    memset(options, 0, sizeof(pomelo_unrolled_list_options_t));
}


pomelo_unrolled_list_t * pomelo_unrolled_list_create(
    pomelo_unrolled_list_options_t * options
) {
    assert(options != NULL);
    if (options->element_size == 0) {
        return NULL;
    }

    pomelo_allocator_t * allocator = options->allocator;
    if (!allocator) {
        allocator = pomelo_allocator_default();
    }

    pomelo_unrolled_list_t * list =
        pomelo_allocator_malloc_t(allocator, pomelo_unrolled_list_t);
    if (!list) {
        // Failed to allocate new list
        return NULL;
    }

    int ret = pomelo_unrolled_list_init(list, options);
    if (ret < 0) {
        pomelo_unrolled_list_destroy(list);
        return NULL;
    }

    return list;
}


void pomelo_unrolled_list_destroy(pomelo_unrolled_list_t * list) {
    assert(list != NULL);
    pomelo_unrolled_list_finalize(list);
    pomelo_allocator_free(list->allocator, list);
}


int pomelo_unrolled_list_init(
    pomelo_unrolled_list_t * list,
    pomelo_unrolled_list_options_t * options
) {
    assert(list != NULL);
    assert(options != NULL);

    pomelo_allocator_t * allocator = options->allocator;
    if (!allocator) {
        allocator = pomelo_allocator_default();
    }
    
    size_t bucket_elements = options->bucket_elements;
    if (bucket_elements == 0) {
        bucket_elements = POMELO_UNROLLED_LIST_DEFAULT_ELEMENTS_PER_BUCKET;
    }

    memset(list, 0, sizeof(pomelo_unrolled_list_t));
    list->allocator = allocator;
    list->element_size = options->element_size;
    list->bucket_elements = bucket_elements;

    size_t bucket_size = options->element_size * bucket_elements;

    pomelo_list_options_t list_options;
    pomelo_list_options_init(&list_options);
    list_options.allocator = allocator;
    list_options.element_size = bucket_size;

    list->nodes = pomelo_list_create(&list_options);
    if (!list->nodes) {
        // Failed to create nodes list
        return -1;
    }

    return 0;
}


void pomelo_unrolled_list_finalize(pomelo_unrolled_list_t * list) {
    assert(list != NULL);

    if (list->nodes) {
        pomelo_list_destroy(list->nodes);
        list->nodes = NULL;
    }
}


int pomelo_unrolled_list_resize(pomelo_unrolled_list_t * list, size_t size) {
    assert(list != NULL);
    
    // The number of bucket
    size_t bucket_size = POMELO_CEIL_DIV(size, list->bucket_elements);
    int ret = pomelo_list_resize(list->nodes, bucket_size);
    if (ret < 0) {
        return -1;
    }

    list->size = size;
    return 0;
}


void pomelo_unrolled_list_clear(pomelo_unrolled_list_t * list) {
    assert(list != NULL);

    pomelo_list_clear(list->nodes);
    list->size = 0;
}


int pomelo_unrolled_list_get(
    pomelo_unrolled_list_t * list,
    size_t index,
    void * p_element
) {
    assert(list != NULL);
    assert(p_element != NULL);

    if (index >= list->size) {
        // Out of bound
        return -1;
    }

    uint8_t * bucket = pomelo_unrolled_list_find_bucket(list, index);
    size_t offset = (index % list->bucket_elements) * list->element_size;
    memcpy(p_element, bucket + offset, list->element_size);

    return 0;
}


int pomelo_unrolled_list_get_back(
    pomelo_unrolled_list_t * list,
    void * p_element
) {
    assert(list != NULL);
    assert(p_element != NULL);

    if (list->size == 0) {
        return -1;
    }

    uint8_t * bucket = pomelo_list_pelement(list->nodes->back);
    size_t offset_index = ((list->size - 1) % list->bucket_elements);
    size_t offset = offset_index * list->element_size;
    memcpy(p_element, bucket + offset, list->element_size);

    return 0;
}


int pomelo_unrolled_list_get_front(
    pomelo_unrolled_list_t * list,
    void * p_element
) {
    assert(list != NULL);
    assert(p_element != NULL);

    if (list->size == 0) {
        return -1;
    }

    uint8_t * bucket = pomelo_list_pelement(list->nodes->front);
    memcpy(p_element, bucket, list->element_size);

    return 0;
}


int pomelo_unrolled_list_set_p(
    pomelo_unrolled_list_t * list,
    size_t index,
    void * p_element
) {
    assert(list != NULL);
    assert(p_element != NULL);

    if (index >= list->size) {
        return -1;
    }

    uint8_t * bucket = pomelo_unrolled_list_find_bucket(list, index);
    size_t offset = (index % list->bucket_elements) * list->element_size;
    memcpy(bucket + offset, p_element, list->element_size);

    return 0;
}


int pomelo_unrolled_list_push_back_p(
    pomelo_unrolled_list_t * list,
    void * p_element
) {
    assert(list != NULL);
    assert(p_element != NULL);

    size_t new_size = list->size + 1;
    size_t new_bucket_size = POMELO_CEIL_DIV(new_size, list->bucket_elements);
    
    if (new_bucket_size != list->nodes->size) {
        int ret = pomelo_list_resize(list->nodes, new_bucket_size);
        if (ret < 0) {
            return -1;
        }
    }

    uint8_t * bucket = pomelo_list_pelement(list->nodes->back);
    size_t offset = (list->size % list->bucket_elements) * list->element_size;
    memcpy(bucket + offset, p_element, list->element_size);

    list->size++;
    return 0;
}


int pomelo_unrolled_list_pop_back(
    pomelo_unrolled_list_t * list,
    void * p_element
) {
    assert(list != NULL);

    if (list->size == 0) {
        // List is empty
        return -1;
    }

    if (p_element) {
        uint8_t * bucket = pomelo_list_pelement(list->nodes->back);
        size_t offset_index = ((list->size - 1) % list->bucket_elements);
        size_t offset = offset_index * list->element_size;
        memcpy(p_element, bucket + offset, list->element_size);
    }

    size_t new_size = list->size - 1;
    size_t new_bucket_size = POMELO_CEIL_DIV(new_size, list->bucket_elements);
    if (new_bucket_size != list->nodes->size) {
        int ret = pomelo_list_resize(list->nodes, new_bucket_size);
        if (ret < 0) {
            return -1;
        }
    }

    list->size--;
    return 0;
}


void pomelo_unrolled_list_begin(
    pomelo_unrolled_list_t * list,
    pomelo_unrolled_list_iterator_t * it
) {
    assert(list != NULL);
    assert(it != NULL);

    it->list = list;

    if (list->size == 0) {
        it->node = NULL;
        it->bucket = NULL;
        it->index = 0;
        return;
    }

    it->node = list->nodes->front;
    it->bucket = pomelo_list_pelement(it->node);
    it->index = 0;
}


void pomelo_unrolled_list_end(
    pomelo_unrolled_list_t * list,
    pomelo_unrolled_list_iterator_t * it
) {
    assert(list != NULL);
    assert(it != NULL);

    it->list = list;

    if (list->size == 0) {
        it->node = NULL;
        it->bucket = NULL;
        it->index = 0;
        return;
    }

    it->node = list->nodes->back;
    it->bucket = pomelo_list_pelement(it->node);
    it->index = list->size - 1;
}


void * pomelo_unrolled_list_iterator_next(
    pomelo_unrolled_list_iterator_t * it,
    void * output
) {
    assert(it != NULL);
    if (it->index >= it->list->size) {
        // End of iteration
        return NULL;
    }


    pomelo_unrolled_list_t * list = it->list;
    size_t bucket_index = it->index % list->bucket_elements;

    void * element = it->bucket + bucket_index * list->element_size;
    if (output) {
        memcpy(output, element, list->element_size);
    }

    if (bucket_index == list->bucket_elements - 1) {
        // Next bucket
        it->node = it->node->next;
        it->bucket = it->node ? pomelo_list_pelement(it->node) : NULL;
    }

    ++it->index;
    return element;
}


void * pomelo_unrolled_list_iterator_prev(
    pomelo_unrolled_list_iterator_t * it,
    void * output
) {
    assert(it != NULL);
    if (it->index >= it->list->size) {
        return NULL;
    }

    pomelo_unrolled_list_t * list = it->list;
    size_t bucket_index = it->index % list->bucket_elements;
    void * element = it->bucket + bucket_index * list->element_size;
    if (output) {
        memcpy(output, element, list->element_size);
    }

    if (bucket_index == 0) {
        // Previous bucket
        it->node = it->node->prev;
        it->bucket = it->node ? pomelo_list_pelement(it->node) : NULL;
    }

    --it->index;
    return element;
}
