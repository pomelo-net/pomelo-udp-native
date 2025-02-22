#include <assert.h>
#include "mutex.h"
#include "uv.h"


struct pomelo_mutex_s {
    /// @brief Allocator
    pomelo_allocator_t * allocator;

    /// @brief UV mutex
    uv_mutex_t uv_mutex;
};


pomelo_mutex_t * pomelo_mutex_create(pomelo_allocator_t * allocator) {
    assert(allocator != NULL);
    pomelo_mutex_t * mutex =
        pomelo_allocator_malloc_t(allocator, pomelo_mutex_t);
    if (!mutex) {
        return NULL;
    }

    mutex->allocator = allocator;
    int ret = uv_mutex_init_recursive(&mutex->uv_mutex);
    if (ret < 0) {
        pomelo_allocator_free(allocator, mutex);
        return NULL;
    }

    return mutex;
}


void pomelo_mutex_destroy(pomelo_mutex_t * mutex) {
    assert(mutex != NULL);
    uv_mutex_destroy(&mutex->uv_mutex);
    pomelo_allocator_free(mutex->allocator, mutex);
}


void pomelo_mutex_lock(pomelo_mutex_t * mutex) {
    assert(mutex != NULL);
    uv_mutex_lock(&mutex->uv_mutex);
}


void pomelo_mutex_unlock(pomelo_mutex_t * mutex) {
    assert(mutex != NULL);
    uv_mutex_unlock(&mutex->uv_mutex);
}
