#ifndef POMELO_UTILS_MUTEX_SRC_H
#define POMELO_UTILS_MUTEX_SRC_H
#include "pomelo/allocator.h"
#ifdef __cplusplus
extern "C" {
#endif

/// @brief Mutex
typedef struct pomelo_mutex_s pomelo_mutex_t;

/// @brief Create new mutex
pomelo_mutex_t * pomelo_mutex_create(pomelo_allocator_t * allocator);

/// @brief Destroy a mutex
void pomelo_mutex_destroy(pomelo_mutex_t * mutex);

/// @brief Lock mutex
void pomelo_mutex_lock(pomelo_mutex_t * mutex);

/// @brief Unlock mutex
void pomelo_mutex_unlock(pomelo_mutex_t * mutex);


#define POMELO_BEGIN_CRITICAL_SECTION(mutex) if (mutex) pomelo_mutex_lock(mutex)
#define POMELO_END_CRITICAL_SECTION(mutex) if (mutex) pomelo_mutex_unlock(mutex)


#ifdef __cplusplus
}
#endif
#endif // POMELO_UTILS_MUTEX_SRC_H
