#ifndef POMELO_PROTOCOL_EMITTER_SRC_H
#define POMELO_PROTOCOL_EMITTER_SRC_H
#include <stdbool.h>
#include "platform/platform.h"
#include "base/packet.h"
#include "codec/packet.h"
#include "protocol.h"
#include "peer.h"


#ifdef __cplusplus
extern "C" {
#endif


#define POMELO_PROTOCOL_EMITTER_FLAG_RUNNING     (1 << 0) // Emitter is running
#define POMELO_PROTOCOL_EMITTER_FLAG_PROCESSING  (1 << 1) // Emitter is encoding
#define POMELO_PROTOCOL_EMITTER_FLAG_ENCODE_ONCE (1 << 2) // Encode once only
#define POMELO_PROTOCOL_EMITTER_FLAG_ENCODED     (1 << 3) // Encoded packet


/// @brief The packet frequent emitter.
/// This structure describes about sending a kind of packet frequently.
/// Useful for request, response & ping in client mode.
/// It is used by disconnect packet as well.
typedef struct pomelo_protocol_emitter_s pomelo_protocol_emitter_t;


/// @brief The callback when the emitter has triggered
typedef void (*pomelo_protocol_emitter_cb)(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_emitter_t * emitter
);


struct pomelo_protocol_emitter_s {
    /* ------ Public ------ */

    /// @brief The socket
    pomelo_protocol_socket_t * socket;

    /// @brief The packet
    pomelo_packet_t * packet;

    /// @brief The target peer
    pomelo_protocol_peer_t * peer;

    /// @brief The frequency of emitter
    uint64_t frequency;

    /// @brief The callback when this emitter is triggering
    pomelo_protocol_emitter_cb trigger_cb;

    /// @brief The callback when this emitter has timed out.
    pomelo_protocol_emitter_cb timeout_cb;

    /// @brief This callback is called when the counter has reached limit.
    /// If no limit is provided, this will not be involked.
    pomelo_protocol_emitter_cb limit_reached_cb;

    /// @brief The number of times to trigger.
    /// If zero is set, this will trigger forever until it's been stopped.
    uint32_t limit;

    /// @brief The timeout of emitter to run.
    /// If zero is set, the emitter will have no timeout
    uint64_t timeout_ms;

    /* ------ Private ------ */

    /// @brief Running flag. The work has to be stopped if this flag set to 0.
    uint8_t flags;

    /// @brief The triggering timer
    pomelo_platform_timer_t * trigger_timer;

    /// @brief The timer of timing out
    pomelo_platform_timer_t * timeout_timer;

    /// @brief The result of encode function
    int encode_result;

    /// @brief The counter of triggered times.
    /// This will be reset right after starting
    uint32_t trigger_counter;
};


/* -------------------------------------------------------------------------- */
/*                            Frequent emitter                                */
/* -------------------------------------------------------------------------- */

/// @brief Initialize the frequent emitter
void pomelo_protocol_emitter_init(pomelo_protocol_emitter_t * emitter);

/// @brief Start the frequent emitter.
/// Calling start without initializing will lead to undefined behavior
/// @return Returns 0 on success, or an error code < 0 on failure
int pomelo_protocol_emitter_start(pomelo_protocol_emitter_t * emitter);


/// @brief The timer callback for frequent emitter
void pomelo_protocol_emitter_on_triggered(
    pomelo_protocol_emitter_t * emitter
);

/// @brief Timed out callback
void pomelo_protocol_emitter_on_timed_out(
    pomelo_protocol_emitter_t * emitter
);

/// @brief The callback after sending packet
void pomelo_protocol_emitter_on_sent(pomelo_protocol_emitter_t * emitter);

/// @brief Stop the frequent emitter
/// @return Returns 0 on success, or an error code < 0 on failure
int pomelo_protocol_emitter_stop(pomelo_protocol_emitter_t * emitter);

/// @brief Set encode once flag for emitter
void pomelo_protocol_emitter_set_encode_once(
    pomelo_protocol_emitter_t * emitter
);

#ifdef __cplusplus
}
#endif
#endif // POMELO_PROTOCOL_EMITTER_SRC_H
