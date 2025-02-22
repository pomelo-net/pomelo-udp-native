#ifndef POMELO_PLATFORM_POLL_H
#define POMELO_PLATFORM_POLL_H
#include "pomelo/allocator.h"
#include "pomelo/platform.h"
#ifdef __cplusplus
extern "C" {
#endif


/// @brief Options of platform
typedef struct pomelo_platform_poll_options_s pomelo_platform_poll_options_t;


struct pomelo_platform_poll_options_s {
    /// @brief Allocator
    pomelo_allocator_t * allocator;
};


/// @brief Initialize options
void pomelo_platform_poll_options_init(
    pomelo_platform_poll_options_t * options
);


/// @brief Create poll platform
pomelo_platform_t * pomelo_platform_poll_create(
    pomelo_platform_poll_options_t * options
);


/// @brief Destroy poll platform
void pomelo_platform_poll_destroy(pomelo_platform_t * platform);


/// @brief Poll & execute events of platform
/// @returns Zero if done, positive value if platform need to poll more and
/// negative value in case of failure.
int pomelo_platform_poll_service(pomelo_platform_t * platform);


#ifdef __cplusplus
}
#endif
#endif // POMELO_PLATFORM_POLL_H
