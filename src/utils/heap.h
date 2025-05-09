#ifndef POMELO_UTILS_HEAP_SRC_H
#define POMELO_UTILS_HEAP_SRC_H
#include "pomelo/allocator.h"
#include "pool.h"
#include "mutex.h"
#ifdef __cplusplus
extern "C" {
#endif


/// @brief The heap
typedef struct pomelo_heap_s pomelo_heap_t;

/// @brief The options of heap
typedef struct pomelo_heap_options_s pomelo_heap_options_t;

/// @brief The node of heap
typedef struct pomelo_heap_node_s pomelo_heap_node_t;

/// @brief The entry of heap
typedef struct pomelo_heap_entry_s pomelo_heap_entry_t;

/// @brief The compare function
typedef int (*pomelo_heap_compare_fn)(void * a, void * b);


struct pomelo_heap_options_s {
    /// @brief The allocator
    pomelo_allocator_t * allocator;

    /// @brief The size of element
    size_t element_size;

    /// @brief The compare function
    pomelo_heap_compare_fn compare;

    /// @brief The synchronized option
    bool synchronized;
};


struct pomelo_heap_node_s {
    /// @brief The entry
    pomelo_heap_entry_t * entry;

    /// @brief The parent node
    pomelo_heap_node_t * parent;

    /// @brief The left child node
    pomelo_heap_node_t * left;

    /// @brief The right child node
    pomelo_heap_node_t * right;

    /// @brief The next node (for clear operation)
    pomelo_heap_node_t * next;

#ifndef NDEBUG
    /// @brief The signature for debugging
    int signature;
#endif

};


struct pomelo_heap_entry_s {
    /// @brief The node
    pomelo_heap_node_t * node;

    /// @brief The element
    void * element;
};


struct pomelo_heap_s {
    /// @brief The allocator
    pomelo_allocator_t * allocator;

    /// @brief The size of value
    size_t element_size;

    /// @brief The compare function
    pomelo_heap_compare_fn compare;

    /// @brief The root node
    pomelo_heap_node_t * root;

    /// @brief The number of elements
    size_t size;

    /// @brief The pool of nodes
    pomelo_pool_t * node_pool;

    /// @brief The pool of entries
    pomelo_pool_t * entry_pool;

    /// @brief The mutex for synchronized heap
    pomelo_mutex_t * mutex;

#ifndef NDEBUG
    /// @brief The signature for all heaps
    int signature;

    /// @brief The signature for all nodes of this heap
    int node_signature;
#endif

};


/* -------------------------------------------------------------------------- */
/*                               Public APIs                                  */
/* -------------------------------------------------------------------------- */


/// @brief Create a heap
pomelo_heap_t * pomelo_heap_create(pomelo_heap_options_t * options);


/// @brief Destroy a heap
void pomelo_heap_destroy(pomelo_heap_t * heap);


/// @brief Push a element into the heap
pomelo_heap_entry_t * pomelo_heap_push_ptr(
    pomelo_heap_t * heap,
    void * p_element
);


/// @brief Push a element into the heap
#define pomelo_heap_push(heap, element) pomelo_heap_push_ptr(heap, &(element))


/// @brief Pop a element from the heap
int pomelo_heap_pop(pomelo_heap_t * heap, void * p_element);


/// @brief Get the top element of the heap
int pomelo_heap_top(pomelo_heap_t * heap, void * p_element);


/// @brief Get the size of the heap
size_t pomelo_heap_size(pomelo_heap_t * heap);


/// @brief Remove an element from the heap
void pomelo_heap_remove(pomelo_heap_t * heap, pomelo_heap_entry_t * entry);


/// @brief Clear the heap
void pomelo_heap_clear(pomelo_heap_t * heap);


/* -------------------------------------------------------------------------- */
/*                              Internal APIs                                 */
/* -------------------------------------------------------------------------- */


/// @brief Find the insert position of the node.
/// @return The parent node of the insert position.
pomelo_heap_node_t * pomelo_heap_find_insert_position(pomelo_heap_t * heap);


/// @brief Get the last node of the heap
/// @return The last node of the heap
pomelo_heap_node_t * pomelo_heap_find_last_node(pomelo_heap_t * heap);


/// @brief Heapify up the node
void pomelo_heap_heapify_up(pomelo_heap_t * heap, pomelo_heap_node_t * node);


/// @brief Heapify down the node
void pomelo_heap_heapify_down(pomelo_heap_t * heap, pomelo_heap_node_t * node);


/// @brief Remove a node from the heap
void pomelo_heap_remove_node(pomelo_heap_t * heap, pomelo_heap_node_t * node);


#ifdef __cplusplus
}
#endif
#endif // POMELO_UTILS_HEAP_SRC_H
