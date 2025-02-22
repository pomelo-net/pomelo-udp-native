#ifndef POMELO_UTILS_LIST_SRC_H
#define POMELO_UTILS_LIST_SRC_H
#include "pool.h"

#ifdef __cplusplus
extern "C" {
#endif


/// @brief The list, implemented by double linked list
typedef struct pomelo_list_s pomelo_list_t;

/// @brief The list node
typedef struct pomelo_list_node_s pomelo_list_node_t;

/// @brief Creating options of list
typedef struct pomelo_list_options_s pomelo_list_options_t;

/// @brief The list iterator
typedef struct pomelo_list_iterator_s pomelo_list_iterator_t;


struct pomelo_list_node_s {
    /// @brief The next node
    struct pomelo_list_node_s * next;

    /// @brief The current node
    struct pomelo_list_node_s * prev;

#ifndef NDEBUG
    /// @brief The signature for debugging
    int signature;
#endif

    /// Hidden field: element
};


struct pomelo_list_s {
    /// @brief The allocator of list
    pomelo_allocator_t * allocator;

    /// @brief The list size
    size_t size;

    /// @brief The front of list
    pomelo_list_node_t * front;

    /// @brief The last of list
    pomelo_list_node_t * back;

    /// @brief The size of element
    size_t element_size;

    /* Private */

    /// @brief The pool of node
    pomelo_pool_t * node_pool;

    /// @brief Mutex for this pool
    pomelo_mutex_t * mutex;

    /// @brief Modification count
    uint64_t mod_count;

#ifndef NDEBUG
    /// @brief The signature for all lists
    int signature;

    /// @brief The signature for debugging
    int node_signature;
#endif

};


struct pomelo_list_options_s {
    /// @brief The size of element
    size_t element_size;

    /// @brief The allocator. If NULL is set, default allocator will be used.
    pomelo_allocator_t * allocator;

    /// @brief Create a threadsafe list
    bool synchronized;
};


struct pomelo_list_iterator_s {
    /// @brief The list
    pomelo_list_t * list;

    /// @brief The current node
    pomelo_list_node_t * current;

    /// @brief The next node
    pomelo_list_node_t * next;

    /// @brief Modification count
    uint64_t mod_count;
};


/// @brief Set the default configuration for list options
void pomelo_list_options_init(pomelo_list_options_t * options);


/// @brief Create a new list
pomelo_list_t * pomelo_list_create(pomelo_list_options_t * options);

/// @brief Destroy a list
void pomelo_list_destroy(pomelo_list_t * list);

/// @brief Resize list. All new allocated element will be zero-initialized
/// New appended elements will be zero-initialized.
int pomelo_list_resize(pomelo_list_t * list, size_t size);

/// @brief Push an element to front of list (pointer version)
/// @param p_element Pointer to element to append
pomelo_list_node_t * pomelo_list_push_front_p(
    pomelo_list_t * list, void * p_element
);


/// @brief Push an element to front of list
/// @param p_element Pointer to element to append
/// @return New created node
#define pomelo_list_push_front(list, element)                                  \
    pomelo_list_push_front_p(list, &element)


/// @brief Pop an element from front of list and store the data to the second
/// argument.
/// @param list The list
/// @param data The output data block. If NULL is provided, data assigment will
/// be discarded
/// @return Returns on success, or -1 if list is empty
int pomelo_list_pop_front(pomelo_list_t * list, void * data);

/// @brief Push an element to back of list (pointer version)
/// @param p_element Pointer to element to append
pomelo_list_node_t * pomelo_list_push_back_p(
    pomelo_list_t * list, void * p_element
);

/// @brief Push an element to back of list (pointer version)
/// @param p_element Pointer to element to append
/// @return New created node
#define pomelo_list_push_back(list, element)                                   \
    pomelo_list_push_back_p(list, &element)

/// @brief Retrive and remove the element at the back of list
/// @param list The list
/// @param data The output data block. If NULL is provided, data assigment will
/// be discarded
/// @return Returns on success, or an error code < 0 on failure
int pomelo_list_pop_back(pomelo_list_t * list, void * data);

/// @brief Remove a node from list
/// @brief Return 0 on success or an error code < 0 on failure
void pomelo_list_remove(pomelo_list_t * list, pomelo_list_node_t * node);

/// @brief Clear the list
void pomelo_list_clear(pomelo_list_t * list);

/// @brief Check if list is empty
bool pomelo_list_is_empty(pomelo_list_t * list);

/// @brief Get the element from node
/// @param node The list node
/// @param type The type of list element
#define pomelo_list_element(node, type) (*((type*) ((node) + 1)))

/// @brief Get element address from node
#define pomelo_list_pelement(node) ((void *)((node) + 1))

/// @brief For each node of list.
/// Warn: Trying to remove a node in the loop will break it out. Use iterator
/// instead.
#define pomelo_list_for_each(list, node)                                       \
    for (node = (list)->front; node; node = node->next)


/// @brief For each node of list
#define pomelo_list_for(list, element, element_type, statements)               \
do {                                                                           \
    pomelo_list_node_t * _node_ ## element = NULL;                             \
    pomelo_list_for_each(list, _node_ ## element) {                            \
        element = pomelo_list_element(_node_## element, element_type);         \
        do statements while (0);                                               \
    }                                                                          \
} while (0)

/// @brief For each node of list of pointers. This only works for list of
/// pointers.
#define pomelo_list_ptr_for(list, element, statements)                         \
    pomelo_list_for(list, element, void *, statements)


/// @brief Start iterating the list
void pomelo_list_iterate(pomelo_list_t * list, pomelo_list_iterator_t * it);

/// @brief Get the next element
void * pomelo_list_iterator_next(pomelo_list_iterator_t * it, void * p_element);

/// @brief Remove the element at current position
void pomelo_list_iterator_remove(pomelo_list_iterator_t * it);


/* -------------------------------------------------------------------------- */
/*                               Unrolled list                                */
/* -------------------------------------------------------------------------- */

#define POMELO_UNROLLED_LIST_DEFAULT_ELEMENTS_PER_BUCKET 16

struct pomelo_unrolled_list_s;
struct pomelo_unrolled_list_options_s;
struct pomelo_unrolled_list_iterator_s;


/// @brief The unrolled list
typedef struct pomelo_unrolled_list_s pomelo_unrolled_list_t;

/// @brief The unrolled list options
typedef struct pomelo_unrolled_list_options_s pomelo_unrolled_list_options_t;

/// @brief The unrolled list iterator
typedef struct pomelo_unrolled_list_iterator_s pomelo_unrolled_list_iterator_t;


struct pomelo_unrolled_list_s {
    /// @brief The allocator
    pomelo_allocator_t * allocator;

    /// @brief The nodes of list
    pomelo_list_t * nodes;

    /// @brief The size of list
    size_t size;

    /// @brief The bytes of an element
    size_t element_size;

    /// @brief The number of elements per bucket (a node of list)
    size_t bucket_elements;
};


struct pomelo_unrolled_list_options_s {
    /// @brief The allocator of list
    pomelo_allocator_t * allocator;

    /// @brief The bytes of an element
    size_t element_size;

    /// @brief The number of elements per bucket (a node of list)
    size_t bucket_elements;
};


struct pomelo_unrolled_list_iterator_s {
    /// @brief The list
    pomelo_unrolled_list_t * list;

    /// @brief The current node in unrolled list
    pomelo_list_node_t * node;

    /// @brief The current array (associated with node)
    uint8_t * bucket;

    /// @brief The index of element (global)
    size_t index;
};


/// @brief Initialize unrolled list options
void pomelo_unrolled_list_options_init(
    pomelo_unrolled_list_options_t * options
);


/// @brief Create new unrolled list
pomelo_unrolled_list_t * pomelo_unrolled_list_create(
    pomelo_unrolled_list_options_t * options
);


/// @brief Destroy an unrolled list
void pomelo_unrolled_list_destroy(pomelo_unrolled_list_t * list);


/* -------------------------------------------------------------------------- */
/*                The compatible APIs for working with pools                  */
/* -------------------------------------------------------------------------- */

/// @brief Initialize the list.
int pomelo_unrolled_list_init(
    pomelo_unrolled_list_t * list,
    pomelo_unrolled_list_options_t * options
);

/// @brief Finalize the list.
void pomelo_unrolled_list_finalize(pomelo_unrolled_list_t * list);

/* -------------------------------------------------------------------------- */


/// @brief Resize the list. All new allocated element will be zero-initialized
/// @return Returns 0 on success or -1 if failed to allocate.
int pomelo_unrolled_list_resize(pomelo_unrolled_list_t * list, size_t size);


/// @brief Clear unrolled list
void pomelo_unrolled_list_clear(pomelo_unrolled_list_t * list);


/// @brief Get the element at specific index to output
/// @return Returns 0 on success or -1 if index if out of bound.
int pomelo_unrolled_list_get(
    pomelo_unrolled_list_t * list,
    size_t index,
    void * p_element
);


/// @brief Get the last element of list
int pomelo_unrolled_list_get_back(
    pomelo_unrolled_list_t * list,
    void * p_element
);


/// @brief Get the first element of list
int pomelo_unrolled_list_get_front(
    pomelo_unrolled_list_t * list,
    void * p_element
);


/// @brief Set the element at specific index
/// @return Returns 0 on success or -1 if index if out of bound.
int pomelo_unrolled_list_set_p(
    pomelo_unrolled_list_t * list,
    size_t index,
    void * p_element
);


/// @brief Set the element at specific index
/// @return Returns 0 on success or -1 if index if out of bound.
#define pomelo_unrolled_list_set(list, index, element)                         \
    pomelo_unrolled_list_set_p(list, index, &(element))


/// @brief Append an element to the end of list
int pomelo_unrolled_list_push_back_p(
    pomelo_unrolled_list_t * list,
    void * p_element
);


#define pomelo_unrolled_list_push_back(list, element)                          \
    pomelo_unrolled_list_push_back_p(list, &(element))


/// @brief Pop back the list
int pomelo_unrolled_list_pop_back(
    pomelo_unrolled_list_t * list,
    void * p_element
);


/// @brief Iterate the list from the begining of list
void pomelo_unrolled_list_begin(
    pomelo_unrolled_list_t * list,
    pomelo_unrolled_list_iterator_t * it
);


/// @brief Iterate the list from the end of list
void pomelo_unrolled_list_end(
    pomelo_unrolled_list_t * list,
    pomelo_unrolled_list_iterator_t * it
);


/// @brief Get the next element. Copy the element to output (if output not null)
/// @return This function returns a pointer to next element or NULL if there's
/// no more elements
void * pomelo_unrolled_list_iterator_next(
    pomelo_unrolled_list_iterator_t * it,
    void * output
);


/// @brief Get the next element. Copy the element to output (if output not null)
/// @return This function returns a pointer to previous element or NULL if
/// there's no more elements
void * pomelo_unrolled_list_iterator_prev(
    pomelo_unrolled_list_iterator_t * it,
    void * output
);


#ifdef __cplusplus
}
#endif
#endif // POMELO_UTILS_LIST_SRC_H
