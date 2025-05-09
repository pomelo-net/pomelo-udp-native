#ifndef POMELO_PROTOCOL_ADAPTER_H
#define POMELO_PROTOCOL_ADAPTER_H
#include "protocol.h"
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


/// @brief Incoming data record
typedef struct pomelo_protocol_acceptance_s pomelo_protocol_acceptance_t;


struct pomelo_protocol_acceptance_s {
    /// @brief The socket
    pomelo_protocol_socket_t * socket;

    /// @brief The address
    pomelo_address_t address;

    /// @brief The view
    pomelo_buffer_view_t view;

    /// @brief Whether the packet is encrypted
    bool encrypted;

    /// @brief The context
    pomelo_protocol_context_t * context;

    /// @brief The task
    pomelo_sequencer_task_t task;
};


/// @brief Initialize the acceptance
pomelo_protocol_acceptance_t * pomelo_protocol_acceptance_init(
    pomelo_protocol_socket_t * socket,
    pomelo_address_t * address,
    pomelo_buffer_view_t * view,
    bool encrypted
);


/// @brief Process the acceptance
void pomelo_protocol_acceptance_process(
    pomelo_protocol_acceptance_t * acceptance
);


#ifdef __cplusplus
}
#endif // __cplusplus
#endif // POMELO_PROTOCOL_ADAPTER_H
