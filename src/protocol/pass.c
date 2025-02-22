#include <assert.h>
#include <string.h>
#include "utils/pool.h"
#include "codec/packet.h"
#include "socket.h"
#include "pass.h"


/* -------------------------------------------------------------------------- */
/*                              Receiving pass                                */
/* -------------------------------------------------------------------------- */


/// @brief Process callback
static void pomelo_protocol_recv_pass_process_callback(
    pomelo_protocol_recv_pass_t * pass,
    bool canceled
) {
    assert(pass != NULL);
    if (canceled) {
        pass->result = -1; // Canceled
    }

    pomelo_protocol_recv_pass_done(pass);
}


void pomelo_protocol_recv_pass_submit(pomelo_protocol_recv_pass_t * pass) {
    assert(pass != NULL);

    if (pass->flags & POMELO_PROTOCOL_PASS_FLAG_NO_DECRYPT) {
        pass->result = pomelo_codec_decode_packet_body(pass->packet);
        pomelo_protocol_recv_pass_done(pass);
        return; // No decrypt, just decode the body
    }

    // Append to receiving passes list
    pass->node = pomelo_list_push_back(pass->peer->recv_passes, pass);
    if (!pass->node) {
        pass->result = -1;
        pomelo_protocol_recv_pass_done(pass);
        return; // Failed to append to list
    }

    // Submit process to worker
    pomelo_protocol_socket_t * socket = pass->socket;
    int ret = pomelo_platform_submit_worker_task(
        socket->platform,
        socket->task_group,
        (pomelo_platform_task_cb) pomelo_protocol_recv_pass_process,
        (pomelo_platform_task_done_cb)
            pomelo_protocol_recv_pass_process_callback,
        pass
    );

    if (ret < 0) {
        // Failed to submit
        pass->result = ret;
        pomelo_protocol_recv_pass_done(pass);
    }
}


void pomelo_protocol_recv_pass_process(pomelo_protocol_recv_pass_t * pass) {
    assert(pass != NULL);
    pomelo_packet_t * packet = pass->packet;
    // Packet header has been already decoded

    // Decrypt packet first
    int ret = pomelo_codec_decrypt_packet(&pass->peer->codec_ctx, packet);
    if (ret < 0) {
        pass->result = ret;
        return; // Failed to decrypt packet
    }

    // Decode packet body
    pass->result = pomelo_codec_decode_packet_body(packet);
}


void pomelo_protocol_recv_pass_done(pomelo_protocol_recv_pass_t * pass) {
    assert(pass != NULL);

    // Capture all neccessary values before releasing the pass
    pomelo_protocol_socket_t * socket = pass->socket;
    pomelo_packet_t * packet = pass->packet;
    pomelo_protocol_peer_t * peer = pass->peer;
    uint64_t recv_time = pass->recv_time;
    int result = pass->result;
    bool terminated = (pass->flags & POMELO_PROTOCOL_PASS_FLAG_TERMINATED);
    pomelo_list_node_t * node = pass->node;

    // Release the pass
    pomelo_pool_release(socket->pools.recv_pass, pass);
    if (terminated) {
        return; // No more callbacks
    }

    if (node) {
        pomelo_list_remove(peer->recv_passes, node);
    }

    pomelo_protocol_socket_recv_packet(socket, peer, packet, recv_time, result);
}


/* -------------------------------------------------------------------------- */
/*                               Sending pass                                 */
/* -------------------------------------------------------------------------- */


static void pomelo_protocol_send_pass_process_callback(
    pomelo_protocol_send_pass_t * pass,
    bool canceled
) {
    assert(pass != NULL);
    if (canceled) {
        pass->result = -1; // Canceled
    }
    pomelo_protocol_send_pass_process_done(pass);
}


void pomelo_protocol_send_pass_submit(pomelo_protocol_send_pass_t * pass) {
    assert(pass != NULL);

    if (pass->flags & POMELO_PROTOCOL_PASS_FLAG_NO_PROCESS) {
        pomelo_protocol_send_pass_process_done(pass);
        return; // No processing, just pass to the next step
    }

    if (pass->flags & POMELO_PROTOCOL_PASS_FLAG_NO_ENCRYPT) {
        // Just encode the packet
        pomelo_packet_t * packet = pass->packet;
        int ret = pomelo_codec_encode_packet_header(packet);
        if (ret < 0) {
            pass->result = ret;
            pomelo_protocol_send_pass_process_done(pass);
            return; // Failed to encode
        }

        // Then encode body
        ret = pomelo_codec_encode_packet_body(packet);
        if (ret < 0) {
            pass->result = ret;
            pomelo_protocol_send_pass_process_done(pass);
            return; // Failed to encode packet body
        }

        pomelo_protocol_send_pass_process_done(pass);
        return;
    }

    // Append pass to sending passes list of peer
    pass->node = pomelo_list_push_back(pass->peer->send_passes, pass);
    if (!pass->node) {
        pass->result = -1;
        pomelo_protocol_send_pass_process_done(pass);
        return;
    }

    // Submit process to worker
    pomelo_protocol_socket_t * socket = pass->socket;
    int ret = pomelo_platform_submit_worker_task(
        socket->platform,
        socket->task_group,
        (pomelo_platform_task_cb) pomelo_protocol_send_pass_process,
        (pomelo_platform_task_done_cb)
            pomelo_protocol_send_pass_process_callback,
        pass
    );

    if (ret < 0) {
        // Failed to submit pass
        pass->result = ret;
        pomelo_protocol_send_pass_process_done(pass);
    }
}


void pomelo_protocol_send_pass_process(pomelo_protocol_send_pass_t * pass) {
    assert(pass != NULL);
    pomelo_packet_t * packet = pass->packet;

    // Encode header first
    int ret = pomelo_codec_encode_packet_header(packet);
    if (ret < 0) {
        pass->result = ret;
        return; // Failed to encode packet header
    }

    // Then encode body
    ret = pomelo_codec_encode_packet_body(packet);
    if (ret < 0) {
        pass->result = ret;
        return; // Failed to encode packet body
    }

    // Finally, encrypt packet
    pass->result = pomelo_codec_encrypt_packet(&pass->peer->codec_ctx, packet);
}


void pomelo_protocol_send_pass_process_done(
    pomelo_protocol_send_pass_t * pass
) {
    assert(pass != NULL);

    if (pass->flags & POMELO_PROTOCOL_PASS_FLAG_TERMINATED) {
        pomelo_protocol_send_pass_done(pass);
        return; // Pass has been terminated, just call the final callback
    }

    if (pass->result < 0) {
        pomelo_protocol_send_pass_done(pass);
        return; // Processing pass failed
    }

    pomelo_protocol_socket_t * socket = pass->socket;

    // Address only available for server
    pomelo_address_t * address = NULL;
    if (socket->mode == POMELO_PROTOCOL_SOCKET_MODE_SERVER) {
        address = &pass->peer->address;
    }

    int ret = pomelo_adapter_send(
        socket->adapter,
        address,
        pass->packet,
        pass,
        !(pass->flags & POMELO_PROTOCOL_PASS_FLAG_NO_ENCRYPT)
    );
    if (ret < 0) {
        // Failed to delivery packet
        pass->result = ret;
        pomelo_protocol_send_pass_done(pass);
    }
    // => pomelo_protocol_send_pass_done
}


void pomelo_protocol_send_pass_done(pomelo_protocol_send_pass_t * pass) {
    assert(pass != NULL);

    pomelo_protocol_socket_t * socket = pass->socket;
    pomelo_protocol_peer_t * peer = pass->peer;
    pomelo_packet_t * packet = pass->packet;
    int result = pass->result;
    pomelo_list_node_t * node = pass->node;
    bool terminated = pass->flags & POMELO_PROTOCOL_PASS_FLAG_TERMINATED;

    // Release pass
    pomelo_pool_release(socket->pools.send_pass, pass);

    if (terminated) {
        return; // No more callbacks
    }

    if (node) {
        // Remove from sending passes
        pomelo_list_remove(peer->send_passes, node);
    }

    // Involke the callback
    pomelo_protocol_socket_sent_packet(socket, peer, packet, result);
}
