#ifndef POMELO_STATISTIC_H
#define POMELO_STATISTIC_H
#include <stdint.h>
#include "pomelo/statistic/statistic-allocator.h"
#include "pomelo/statistic/statistic-api.h"
#include "pomelo/statistic/statistic-buffer.h"
#include "pomelo/statistic/statistic-platform.h"
#include "pomelo/statistic/statistic-protocol.h"
#include "pomelo/statistic/statistic-delivery.h"

#ifdef __cplusplus
extern "C" {
#endif


struct pomelo_statistic_s {
    /// @brief Allocator statistic
    pomelo_statistic_allocator_t allocator;

    /// @brief Platform statistic
    pomelo_statistic_platform_t platform;

    /// @brief Protocol statistic
    pomelo_statistic_protocol_t protocol;

    /// @brief Transport statistic
    pomelo_statistic_delivery_t delivery;

    /// @brief API statistic
    pomelo_statistic_api_t api;

    /// @brief The buffer context statistic
    pomelo_statistic_buffer_context_t buffer_context;
};


/// @brief The statistic 
typedef struct pomelo_statistic_s pomelo_statistic_t;


#ifdef __cplusplus
}
#endif

#endif // POMELO_STATISTIC_H
