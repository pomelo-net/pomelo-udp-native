#ifndef POMELO_STATISTIC_H
#define POMELO_STATISTIC_H

#include "statistic/statistic-allocator.h"
#include "statistic/statistic-api.h"
#include "statistic/statistic-buffer.h"
#include "statistic/statistic-delivery.h"
#include "statistic/statistic-protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

/// @brief The statistic 
typedef struct pomelo_statistic_s pomelo_statistic_t;

struct pomelo_statistic_s {
    /// @brief Allocator statistic
    pomelo_statistic_allocator_t allocator;

    /// @brief Protocol statistic
    pomelo_statistic_protocol_t protocol;

    /// @brief Transport statistic
    pomelo_statistic_delivery_t delivery;

    /// @brief API statistic
    pomelo_statistic_api_t api;

    /// @brief The buffer statistic
    pomelo_statistic_buffer_t buffer;
};

#ifdef __cplusplus
}
#endif

#endif // POMELO_STATISTIC_H
