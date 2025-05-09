#ifndef POMELO_STATISTIC_PROTOCOL_H
#define POMELO_STATISTIC_PROTOCOL_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @brief The statistic of protocol module
typedef struct pomelo_statistic_protocol_s pomelo_statistic_protocol_t;

struct pomelo_statistic_protocol_s {
    /// @brief The number of active senders
    size_t senders;

    /// @brief The number of active receivers
    size_t receivers;

    /// @brief The number of active packets
    size_t packets;

    /// @brief The number of active peers
    size_t peers;

    /// @brief The number of active servers
    size_t servers;

    /// @brief The number of active clients
    size_t clients;

    /// @brief The number of active crypto contexts
    size_t crypto_contexts;
    
    /// @brief The number of accepted connections
    size_t acceptances;
};

#ifdef __cplusplus
}
#endif

#endif // POMELO_STATISTIC_PROTOCOL_H
