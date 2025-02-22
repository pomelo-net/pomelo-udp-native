#ifndef POMELO_PROTOCOL_CONTEXT_SRC_H
#define POMELO_PROTOCOL_CONTEXT_SRC_H
#include "protocol.h"
#ifdef __cplusplus
extern "C" {
#endif // __plusplus


// The following APIs should be accessed through the interface.

struct pomelo_protocol_context_root_s {
    /// @brief The interface of protocol context
    pomelo_protocol_context_t base;

    /// @brief The allocator
    pomelo_allocator_t * allocator;

    /// @brief [Shared] The buffer context
    pomelo_buffer_context_root_t * buffer_context;
};

#ifdef POMELO_MULTI_THREAD

struct pomelo_protocol_context_shared_s {
    /// @brief The interface of protocol context
    pomelo_protocol_context_t base;

    /// @brief The allocator
    pomelo_allocator_t * allocator;

    /// @brief The root context
    pomelo_protocol_context_root_t * root_context;

    /// @brief The buffer context
    pomelo_buffer_context_shared_t * buffer_context;
};

#endif // !POMELO_MULTI_THREAD

#ifdef __cplusplus
}
#endif // __plusplus
#endif // POMELO_PROTOCOL_CONTEXT_SRC_H
