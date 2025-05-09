#include <assert.h>
#include "mutex.h"
#ifdef _WIN32
#include <Windows.h>
#else
#include <pthread.h>
#endif


#ifndef NDEBUG
#define POMELO_MUTEX_SIGNATURE 0xa8f5e8
#define pomelo_mutex_check_signature(mutex) \
    assert((mutex)->signature == POMELO_MUTEX_SIGNATURE)
#else
#define pomelo_mutex_check_signature(mutex)
#endif



struct pomelo_mutex_s {
    /// @brief Allocator
    pomelo_allocator_t * allocator;

    /// @brief Platform mutex
#ifdef _WIN32
    CRITICAL_SECTION mutex;
#else
    pthread_mutex_t mutex;
#endif

#ifndef NDEBUG
    /// @brief Signature
    int signature;
#endif
};


pomelo_mutex_t * pomelo_mutex_create(pomelo_allocator_t * allocator) {
    assert(allocator != NULL);
    pomelo_mutex_t * mutex =
        pomelo_allocator_malloc_t(allocator, pomelo_mutex_t);
    if (!mutex) return NULL;

#ifndef NDEBUG
    mutex->signature = POMELO_MUTEX_SIGNATURE;
#endif

    mutex->allocator = allocator;
#ifdef _WIN32
    InitializeCriticalSection(&mutex->mutex);
#else

    pthread_mutexattr_t attr;
    int ret = pthread_mutexattr_init(&attr);
    if (ret != 0) {
        pomelo_allocator_free(allocator, mutex);
        return NULL;
    }
    
    ret = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    if (ret != 0) {
        pthread_mutexattr_destroy(&attr);
        pomelo_allocator_free(allocator, mutex);
        return NULL;
    }

    ret = pthread_mutex_init(&mutex->mutex, &attr);
    if (ret != 0) {
        pthread_mutexattr_destroy(&attr);
        pomelo_allocator_free(allocator, mutex);
        return NULL;
    }

    ret = pthread_mutexattr_destroy(&attr);
    if (ret != 0) {
        pomelo_mutex_destroy(mutex);
        pomelo_allocator_free(allocator, mutex);
        return NULL;
    }
#endif

    return mutex;
}


void pomelo_mutex_destroy(pomelo_mutex_t * mutex) {
    assert(mutex != NULL);
    pomelo_mutex_check_signature(mutex);

#ifdef _WIN32
    DeleteCriticalSection(&mutex->mutex);
#else
    pthread_mutex_destroy(&mutex->mutex);
#endif

    pomelo_allocator_free(mutex->allocator, mutex);
}


void pomelo_mutex_lock(pomelo_mutex_t * mutex) {
    assert(mutex != NULL);
    pomelo_mutex_check_signature(mutex);

#ifdef _WIN32
    EnterCriticalSection(&mutex->mutex);
#else
    pthread_mutex_lock(&mutex->mutex);
#endif
}


void pomelo_mutex_unlock(pomelo_mutex_t * mutex) {
    assert(mutex != NULL);
    pomelo_mutex_check_signature(mutex);

#ifdef _WIN32
    LeaveCriticalSection(&mutex->mutex);
#else
    pthread_mutex_unlock(&mutex->mutex);
#endif
}
