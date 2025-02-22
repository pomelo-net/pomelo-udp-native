#ifndef POMELO_DELIVERY_TRANSPORTER_SRC_H
#define POMELO_DELIVERY_TRANSPORTER_SRC_H
#include "pomelo/allocator.h"
#include "platform/platform.h"
#include "utils/pool.h"
#include "utils/list.h"
#include "delivery.h"

#ifdef __cplusplus
extern "C" {
#endif


/// @brief The sending command of transport
typedef struct pomelo_delivery_send_command_s pomelo_delivery_send_command_t;

/// @brief The receiving command of transport
typedef struct pomelo_delivery_recv_command_s pomelo_delivery_recv_command_t;


struct pomelo_delivery_transporter_s {
    /// @brief The extra data for transporter
    void * extra_data;

    /// @brief The allocator
    pomelo_allocator_t * allocator;

    /// @brief The platform
    pomelo_platform_t * platform;

    /// @brief The context of transporter
    pomelo_delivery_context_t * context;

    /// @brief The pool of sending commands
    pomelo_pool_t * send_pool;

    /// @brief The pool of receiving commands
    pomelo_pool_t * recv_pool;

    /// @brief The transporter pool
    pomelo_pool_t * endpoint_pool;

    /// @brief The pool of checksum commands
    pomelo_pool_t * checksum_pool;

    /// @brief The work group of delivery layer
    pomelo_platform_task_group_t * task_group;

    /// @brief The number of buses
    size_t nbuses;

    /// @brief The list of using endpoints
    pomelo_list_t * endpoints;

    /// @brief The destroying request flag
    bool finalizing;

    /// @brief The finalizing data
    void * finalize_data;
};

/// @brief This callback is called after canceling all working jobs
void pomelo_delivery_transporter_stop_deferred(
    pomelo_delivery_transporter_t * transporter
);


#ifdef __cplusplus
}
#endif
#endif // POMELO_DELIVERY_TRANSPORTER_SRC_H
