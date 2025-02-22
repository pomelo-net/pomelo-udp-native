#include <assert.h>
#include <string.h>
#include "utils/macro.h"
#include "pass.h"
#include "socket.h"
#include "emitter.h"


/* -------------------------------------------------------------------------- */
/*                            Frequent emitter                                */
/* -------------------------------------------------------------------------- */

void pomelo_protocol_emitter_init(pomelo_protocol_emitter_t * emitter) {
    assert(emitter != NULL);
    memset(emitter, 0, sizeof(pomelo_protocol_emitter_t));
}


int pomelo_protocol_emitter_start(pomelo_protocol_emitter_t * emitter) {
    assert(emitter != NULL);

    if (emitter->flags & POMELO_PROTOCOL_EMITTER_FLAG_RUNNING) {
        // The emitter has been running
        return -1;
    }

    pomelo_protocol_socket_t * socket = emitter->socket;
    pomelo_platform_t * platform = socket->platform;
    pomelo_packet_t * packet = emitter->packet;

    // Reset the flag & re-attach the buffer
    emitter->flags &= ~POMELO_PROTOCOL_EMITTER_FLAG_ENCODED;
    pomelo_packet_attach_buffer(packet, packet->buffer);

    emitter->trigger_timer = pomelo_platform_timer_start(
        platform,
        (pomelo_platform_timer_cb) pomelo_protocol_emitter_on_triggered,
        0, // No timeout
        POMELO_FREQ_TO_MS(emitter->frequency),
        emitter
    );
    
    if (!emitter->trigger_timer) {
        return -1;
    }

    if (emitter->timeout_ms > 0) {
        emitter->timeout_timer = pomelo_platform_timer_start(
            platform,
            (pomelo_platform_timer_cb) pomelo_protocol_emitter_on_timed_out,
            emitter->timeout_ms,
            0, // No repeat
            emitter
        );

        if (!emitter->timeout_timer) {
            pomelo_platform_timer_stop(platform, emitter->trigger_timer);
            emitter->trigger_timer = NULL;
            return -1;
        }
    }

    emitter->trigger_counter = 0;
    emitter->flags |= POMELO_PROTOCOL_EMITTER_FLAG_RUNNING;
    return 0;
}


void pomelo_protocol_emitter_on_triggered(
    pomelo_protocol_emitter_t * emitter
) {
    assert(emitter != NULL);

    if (emitter->flags & POMELO_PROTOCOL_EMITTER_FLAG_PROCESSING) {
        return; // The emitter has been processing, ignore current trigger
    }

    pomelo_protocol_socket_t * socket = emitter->socket;
    pomelo_protocol_send_pass_t * pass =
        pomelo_pool_acquire(socket->pools.send_pass);
    if (!pass) {
        return; // Failed to acquire new sending pass
    }
    
    // Set running flag
    emitter->flags |= POMELO_PROTOCOL_EMITTER_FLAG_PROCESSING;

    // Call the callback
    if (emitter->trigger_cb) {
        emitter->trigger_cb(socket, emitter);
    }

    if (emitter->flags & POMELO_PROTOCOL_EMITTER_FLAG_ENCODED) {
        // The packet has been encrypted
        pass->flags |= POMELO_PROTOCOL_PASS_FLAG_NO_PROCESS;
    }

    if (socket->no_encrypt) {
        pass->flags |= POMELO_PROTOCOL_PASS_FLAG_NO_ENCRYPT;
    }

    pass->socket = socket;
    pass->packet = emitter->packet;
    pass->peer = emitter->peer;

    emitter->trigger_counter++;
    if (emitter->limit > 0 && emitter->limit == emitter->trigger_counter) {
        // Reach the limit, stop emitting
        pomelo_protocol_emitter_stop(emitter);

        // Then call the callback
        if (emitter->limit_reached_cb) {
            emitter->limit_reached_cb(emitter->socket, emitter);
        }
    }

    pomelo_protocol_send_pass_submit(pass);
    // => pomelo_protocol_emitter_on_sent
}


void pomelo_protocol_emitter_on_timed_out(
    pomelo_protocol_emitter_t * emitter
) {
    assert(emitter != NULL);

    emitter->timeout_timer = NULL;
    pomelo_protocol_emitter_stop(emitter);

    // Call the callback
    if (emitter->timeout_cb) {
        emitter->timeout_cb(emitter->socket, emitter);
    }
}


void pomelo_protocol_emitter_on_sent(pomelo_protocol_emitter_t * emitter) {
    assert(emitter != NULL);
    emitter->flags &= ~POMELO_PROTOCOL_EMITTER_FLAG_PROCESSING;
    if (emitter->flags & POMELO_PROTOCOL_EMITTER_FLAG_ENCODE_ONCE) {
        emitter->flags |= POMELO_PROTOCOL_EMITTER_FLAG_ENCODED;
    }
}


int pomelo_protocol_emitter_stop(pomelo_protocol_emitter_t * emitter) {
    assert(emitter != NULL);

    if (!(emitter->flags & POMELO_PROTOCOL_EMITTER_FLAG_RUNNING)) {
        return -1;
    }

    pomelo_platform_t * platform = emitter->socket->platform;

    // Just stop & destroy the timer
    pomelo_platform_timer_stop(platform, emitter->trigger_timer);
    emitter->trigger_timer = NULL;

    if (emitter->timeout_timer) {
        pomelo_platform_timer_stop(platform, emitter->timeout_timer);
        emitter->timeout_timer = NULL;
    }

    emitter->flags &= ~POMELO_PROTOCOL_EMITTER_FLAG_RUNNING;
    return 0;
}


void pomelo_protocol_emitter_set_encode_once(
    pomelo_protocol_emitter_t * emitter
) {
    assert(emitter != NULL);
    emitter->flags |= POMELO_PROTOCOL_EMITTER_FLAG_ENCODE_ONCE;
}
