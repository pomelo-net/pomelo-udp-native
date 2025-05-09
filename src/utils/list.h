#ifndef POMELO_UTILS_LIST_SRC_H
#define POMELO_UTILS_LIST_SRC_H
#include "pool.h"

#ifdef __cplusplus
extern "C" {
#endif


/// @brief The list, implemented by double linked list
typedef struct pomelo_list_s pomelo_list_t;

/// @brief The list entry
typedef struct pomelo_list_entry_s pomelo_list_entry_t;

/// @brief Creating options of list
typedef struct pomelo_list_options_s pomelo_list_options_t;

/// @brief The list iterator
typedef struct pomelo_list_iterator_s pomelo_list_iterator_t;

/// @brief The context of list
typedef struct pomelo_list_context_s pomelo_list_context_t;

/// @brief The options of list context
typedef struct pomelo_list_context_options_s pomelo_list_context_options_t;


struct pomelo_list_entry_s {
    /// @brief The next entry
    struct pomelo_list_entry_s * next;

    /// @brief The previous entry
    struct pomelo_list_entry_s * prev;

#ifndef NDEBUG
    /// @brief The signature for debugging
    int signature;
#endif

    /// Hidden field: element
};


struct pomelo_list_context_s {
    /// @brief The allocator of context
    pomelo_allocator_t * allocator;

    /// @brief The size of element
    size_t element_size;

    /// @brief The pool of entry
    pomelo_pool_t * entry_pool;
};


struct pomelo_list_context_options_s {
    /// @brief The allocator of context
    pomelo_allocator_t * allocator;

    /// @brief The size of element
    size_t element_size;
};


struct pomelo_list_s {
    /// @brief The allocator of list
    pomelo_allocator_t * allocator;

    /// @brief The list size
    size_t size;

    /// @brief The front of list
    pomelo_list_entry_t * front;

    /// @brief The last of list
    pomelo_list_entry_t * back;

    /// @brief The context of list
    pomelo_list_context_t * context;

    /// @brief Whether the context is created by this list
    bool context_owned;

    /// @brief Mutex for this pool
    pomelo_mutex_t * mutex;

    /// @brief Modification count
    uint64_t mod_count;

#ifndef NDEBUG
    /// @brief The signature for all lists
    int signature;

    /// @brief The signature for all nodes of this list
    int entry_signature;
#endif

};


struct pomelo_list_options_s {
    /// @brief The size of element. If context is set, this value will be
    /// ignored.
    size_t element_size;

    /// @brief The allocator. If NULL is set, default allocator will be used.
    pomelo_allocator_t * allocator;

    /// @brief The context of list. If NULL is set, new context will be created.
    pomelo_list_context_t * context;

    /// @brief Create a threadsafe list
    bool synchronized;
};


struct pomelo_list_iterator_s {
    /// @brief The list
    pomelo_list_t * list;

    /// @brief The current node
    pomelo_list_entry_t * current;

    /// @brief The next node
    pomelo_list_entry_t * next;

    /// @brief Modification count
    uint64_t mod_count;
};


/// @brief Create a new list context
pomelo_list_context_t * pomelo_list_context_create(
    pomelo_list_context_options_t * options
);


/// @brief Destroy a list context
void pomelo_list_context_destroy(pomelo_list_context_t * context);


/// @brief Create a new list
pomelo_list_t * pomelo_list_create(pomelo_list_options_t * options);


/// @brief Destroy a list
void pomelo_list_destroy(pomelo_list_t * list);


/// @brief Resize list. All new allocated element will be zero-initialized
/// New appended elements will be zero-initialized.
int pomelo_list_resize(pomelo_list_t * list, size_t size);


/// @brief Push an element to front of list (pointer version)
/// @param p_element Pointer to element to append
pomelo_list_entry_t * pomelo_list_push_front_p(
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
/// @return Returns 0 on success, or -1 if list is empty
int pomelo_list_pop_front(pomelo_list_t * list, void * data);

/// @brief Push an element to back of list (pointer version)
/// @param p_element Pointer to element to append
pomelo_list_entry_t * pomelo_list_push_back_ptr(
    pomelo_list_t * list, void * p_element
);

/// @brief Push an element to back of list (pointer version)
/// @param p_element Pointer to element to append
/// @return New created node
#define pomelo_list_push_back(list, element)                                   \
    pomelo_list_push_back_ptr(list, &element)

/// @brief Retrive and remove the element at the back of list
/// @param list The list
/// @param data The output data block. If NULL is provided, data assigment will
/// be discarded
/// @return Returns on success, or an error code < 0 on failure
int pomelo_list_pop_back(pomelo_list_t * list, void * data);

/// @brief Remove a node from list
void pomelo_list_remove(pomelo_list_t * list, pomelo_list_entry_t * entry);

/// @brief Clear the list
void pomelo_list_clear(pomelo_list_t * list);

/// @brief Check if list is empty
bool pomelo_list_is_empty(pomelo_list_t * list);


/// @brief Get the element from entry
#define pomelo_list_element(entry, type) (*((type*) ((entry) + 1)))


/// @brief Initialize the iterator from begin of the list
void pomelo_list_iterator_init(
    pomelo_list_iterator_t * it,
    pomelo_list_t * list
);


/// @brief Get the next element
/// @return Returns 0 on success, -1 if there's no more elements or -2 if the
/// mod count has changed
int pomelo_list_iterator_next(pomelo_list_iterator_t * it, void * p_element);


/// @brief Remove the element at current position
void pomelo_list_iterator_remove(pomelo_list_iterator_t * it);


/// @brief Transfer the element at current position to another list.
/// The target list must have the same context as the source list.
/// @return Returns the transferred element
pomelo_list_entry_t * pomelo_list_iterator_transfer(
    pomelo_list_iterator_t * it,
    pomelo_list_t * list
);

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

    /// @brief The entries of list
    pomelo_list_t * entries;

    /// @brief The size of list
    size_t size;

    /// @brief The bytes of an element
    size_t element_size;

    /// @brief The number of elements per bucket (an entry of list)
    size_t bucket_elements;
};


struct pomelo_unrolled_list_options_s {
    /// @brief The allocator of list
    pomelo_allocator_t * allocator;

    /// @brief The bytes of an element
    size_t element_size;

    /// @brief The number of elements per bucket
    size_t bucket_elements;
};


struct pomelo_unrolled_list_iterator_s {
    /// @brief The list
    pomelo_unrolled_list_t * list;

    /// @brief The current entry in unrolled list
    pomelo_list_entry_t * entry;

    /// @brief The current array (associated with entry)
    uint8_t * bucket;

    /// @brief The index of element (global)
    size_t index;
};


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
/// @return Returns the pointer to the element on success, or NULL if failed
/// to allocate memory.
void * pomelo_unrolled_list_set_ptr(
    pomelo_unrolled_list_t * list,
    size_t index,
    void * p_element
);


/// @brief Set the element at specific index
/// @return Returns 0 on success or -1 if index if out of bound.
#define pomelo_unrolled_list_set(list, index, element)                         \
    pomelo_unrolled_list_set_ptr(list, index, &(element))


/// @brief Append an element to the end of list
void * pomelo_unrolled_list_push_back_ptr(
    pomelo_unrolled_list_t * list,
    void * p_element
);


#define pomelo_unrolled_list_push_back(list, element)                          \
    pomelo_unrolled_list_push_back_ptr(list, &(element))


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
/// @return Returns 0 on success, or -1 if there's no more elements
int pomelo_unrolled_list_iterator_next(
    pomelo_unrolled_list_iterator_t * it,
    void * output
);


/// @brief Get the previous element. Copy the element to output (if output not
/// null)
/// @return Returns 0 on success, or -1 if there's no more elements
int pomelo_unrolled_list_iterator_prev(
    pomelo_unrolled_list_iterator_t * it,
    void * output
);


#ifdef __cplusplus
}
#endif
#endif // POMELO_UTILS_LIST_SRC_H
