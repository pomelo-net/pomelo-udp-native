#ifndef POMELO_STATISTIC_PROTOCOL_H
#define POMELO_STATISTIC_PROTOCOL_H
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif


struct pomelo_statistic_protocol_s {
    /// @brief The number of in-use packets in protocol layer
    size_t packets;

    /// @brief The number of in-use receiving passes
    size_t recv_passes;

    /// @brief The number of in-use sending passes
    size_t send_passes;

    /// @brief The number of bytes of all valid packets
    uint64_t recv_valid_bytes;

    /// @brief The number of bytes of all invalid packets
    uint64_t recv_invalid_bytes;

    /// @brief The number of peers
    size_t peers;
};


/// @brief The statistic of protocol module
typedef struct pomelo_statistic_protocol_s pomelo_statistic_protocol_t;


#ifdef __cplusplus
}
#endif
#endif // POMELO_STATISTIC_PROTOCOL_H
