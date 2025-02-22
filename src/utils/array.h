#ifndef POMELO_UTILS_ARRAY_SRC_H
#define POMELO_UTILS_ARRAY_SRC_H
#include "pomelo/allocator.h"

#ifdef __cplusplus
extern "C" {
#endif


/// The default initial capacity of array
#define POMELO_ARRAY_INIT_CAPACITY 16


/// @brief The pomelo dynamic array
typedef struct pomelo_array_s pomelo_array_t;

/// @brief The array options
typedef struct pomelo_array_options_s pomelo_array_options_t;


struct pomelo_array_s {
    /// @brief The current size of array
    size_t size;

    /// @brief The current capacity of array
    size_t capacity;

    /// @brief The elements array
    void * elements;

    /// @brief The size of element
    size_t element_size;

    /// @brief The allocator
    pomelo_allocator_t * allocator;

#ifndef NDEBUG
    /// @brief The signature for all arrays
    int signature;
#endif

};


struct pomelo_array_options_s {
    /// @brief The allocator
    pomelo_allocator_t * allocator;

    /// @brief The element size
    size_t element_size;

    /// @brief The initial capacity
    size_t initial_capacity;
};


/// Set the default options for array
void pomelo_array_options_init(pomelo_array_options_t * options);


/// @brief Create new dynamic array
/// @param options Array options
/// @return New dynamic array or NULL on failure
pomelo_array_t * pomelo_array_create(pomelo_array_options_t * options);

/// @brief Destroy a dynamic array
/// @param array Array to destroy
void pomelo_array_destroy(pomelo_array_t * array);

/// @brief Ensure size of array
int pomelo_array_ensure_size(pomelo_array_t * array, size_t size);

/// @brief Append an element to the last of array
/// @param p_element Pointer to element to append
/// @return Return 0 on success or negative value on failure
int pomelo_array_append_p(pomelo_array_t * array, void * p_element);

/// Append an element to array
#define pomelo_array_append(array, element)                                    \
    pomelo_array_append_p(array, &(element))

/// @brief Resize the array, new elements will be filled with zeroes
int pomelo_array_resize(pomelo_array_t * array, size_t new_size);

/// @brief Get pointer to element of array by index
/// @param index The index of element
void * pomelo_array_get_p(pomelo_array_t * array, size_t index);

/// @brief Get the value at index
int pomelo_array_get(pomelo_array_t * array, size_t index, void * p_value);

/// @brief Set element at index by its pointer
int pomelo_array_set_p(pomelo_array_t * array, size_t index, void * p_value);

/// Set element at index
#define pomelo_array_set(array, index, value)                                  \
    pomelo_array_set_p(array, index, &(value))

/// Clear all elements of array
#define pomelo_array_clear(array) pomelo_array_resize(array, 0)

#ifdef __cplusplus
}
#endif
#endif // POMELO_UTILS_ARRAY_SRC_H
