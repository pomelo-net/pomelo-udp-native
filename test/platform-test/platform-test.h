#ifndef POMELO_TEST_PLATFORM_TEST_H
#define POMELO_TEST_PLATFORM_TEST_H
#include "pomelo/platform.h"
#include "pomelo/allocator.h"
#ifdef __cplusplus
extern "C" {
#endif


/// @brief Create platform
pomelo_platform_t * pomelo_test_platform_create(pomelo_allocator_t * allocator);


/// @brief Destroy platform
void pomelo_test_platform_destroy(pomelo_platform_t * platform);


/// @brief Execute the platform
void pomelo_test_platform_run(pomelo_platform_t * platform);


#ifdef __cplusplus
}
#endif
#endif // POMELO_TEST_PLATFORM_TEST_H
