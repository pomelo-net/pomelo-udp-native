#ifndef POMELO_STATISTIC_DELIVERY_H
#define POMELO_STATISTIC_DELIVERY_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @brief The statistic of transport module
typedef struct pomelo_statistic_delivery_s pomelo_statistic_delivery_t;

struct pomelo_statistic_delivery_s {
    /// @brief The number of active dispatchers
    size_t dispatchers;

    /// @brief The number of active senders
    size_t senders;

    /// @brief The number of active receivers
    size_t receivers;

    /// @brief The number of active endpoints
    size_t endpoints;

    /// @brief The number of active buses
    size_t buses;

    /// @brief The number of active receptions
    size_t receptions;

    /// @brief The number of active transmissions
    size_t transmissions;

    /// @brief The number of active parcels
    size_t parcels;

    /// @brief The number of active heartbeat objects
    size_t heartbeats;
};

#ifdef __cplusplus
}
#endif

#endif // POMELO_STATISTIC_DELIVERY_H
