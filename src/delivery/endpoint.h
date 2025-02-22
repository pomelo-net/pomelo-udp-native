#ifndef POMELO_DELIVERY_ENDPOINT_SRC_H
#define POMELO_DELIVERY_ENDPOINT_SRC_H
#include "utils/list.h"
#include "delivery.h"
#include "fragment.h"


#ifdef __cplusplus
extern "C" {
#endif

/// The number of sample size
#define POMELO_DELIVERY_RTT_SAMPLE_SIZE 5

/// The number of ping entries of a transporter
#define POMELO_DELIVERY_PING_ENTRIES_BUFFER_SIZE 10


struct pomelo_delivery_endpoint_s {
    /// @brief The extra data of endpoint
    void * extra_data;

    /// @brief The transporter which contains this endpoint
    pomelo_delivery_transporter_t * transporter;

    /// @brief The total number of buses
    size_t nbuses;

    /// @brief The buses array of this endpoint
    pomelo_delivery_bus_t * buses;

    /// @brief The list node for optimization
    pomelo_list_node_t * list_node;
};


/* -------------------------------------------------------------------------- */
/*                            Pool-compatible APIs                            */
/* -------------------------------------------------------------------------- */


/// @brief Initialize the endpoint
int pomelo_delivery_endpoint_init(
    pomelo_delivery_endpoint_t * endpoint,
    pomelo_delivery_transporter_t * transporter
);


/// @brief Reset the endpoint, clear all running commands
int pomelo_delivery_endpoint_reset(
    pomelo_delivery_endpoint_t * endpoint,
    pomelo_delivery_transporter_t * transporter
);


/// @brief Finalize the endpoint
int pomelo_delivery_endpoint_finalize(
    pomelo_delivery_endpoint_t * endpoint,
    pomelo_delivery_transporter_t * transporter
);


#ifdef __cplusplus
}
#endif
#endif // POMELO_DELIVERY_ENDPOINT_SRC_H
