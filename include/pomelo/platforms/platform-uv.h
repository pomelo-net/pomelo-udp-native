#ifndef POMELO_PLATFORM_UV_H
#define POMELO_PLATFORM_UV_H
#include "uv.h"
#include "pomelo/allocator.h"
#include "pomelo/platform.h"

#ifdef __cplusplus
extern "C" {
#endif



/// @brief The options for platform
typedef struct pomelo_platform_uv_options_s pomelo_platform_uv_options_t;


struct pomelo_platform_uv_options_s {
    /// @brief The allocator
    pomelo_allocator_t * allocator;

    /// @brief The UV loop
    uv_loop_t * uv_loop;
};


/// @brief Set the default options for platform
void pomelo_platform_uv_options_init(pomelo_platform_uv_options_t * options);

/// @brief Initialize the platform
pomelo_platform_t * pomelo_platform_uv_create(
    pomelo_platform_uv_options_t * options
);

/// @brief Destroy the platform. After calling this function, if there are some
/// pending works to do, no more callbacks will be called.
void pomelo_platform_uv_destroy(pomelo_platform_t * platform);

/// @brief Get the UV loop of the platform
uv_loop_t * pomelo_platform_uv_get_uv_loop(pomelo_platform_t * platform);


#ifdef __cplusplus
}
#endif
#endif // POMELO_PLATFORM_UV_H
