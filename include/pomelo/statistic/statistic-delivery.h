#ifndef POMELO_STATISTIC_DELIVERY_H
#define POMELO_STATISTIC_DELIVERY_H
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif


struct pomelo_statistic_delivery_s {
    /// @brief The number of in-use endpoint
    size_t endpoints;

    /// @brief The number of in-use sending commands
    size_t send_commands;

    /// @brief The number of in-use receiving commands
    size_t recv_commands;

    /// @brief The number of in-use fragments
    size_t fragments;

    /// @brief The number of in-use messages
    size_t parcels;
};


/// @brief The statistic of transport module
typedef struct pomelo_statistic_delivery_s pomelo_statistic_delivery_t;


#ifdef __cplusplus
}
#endif
#endif // POMELO_STATISTIC_TRANSPORT_H
