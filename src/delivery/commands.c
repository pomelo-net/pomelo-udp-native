#include <assert.h>
#include "utils/macro.h"
#include "commands.h"
#include "bus.h"
#include "endpoint.h"
#include "parcel.h"


/// The maximum alive time of unreliable parcel. The parcel will be
/// auto-released after a certain time but not great than this value.
#define POMELO_UNREL_MSG_MAX_ALIVE_TIME_NS 1000000000ULL // 1s

/// The factor of expired time for unreliable parcel:
/// expired = factor * rtt
#define POMELO_UNREL_MSG_EXPIRED_FACTOR_RTT 10

/// The minimum parcel resending schedule. The parcel will be resent after
/// a certain time but not less than this value.
#define POMELO_REL_MSG_MIN_RESEND_TIME_NS 10000000ULL // 10ms

/// The factor of resending time for reliable parcel:
/// resend = factor * rtt
#define POMELO_REL_MSG_RESENT_FACTOR_RTT 1


pomelo_delivery_recv_command_t * pomelo_delivery_recv_command_prepare(
    pomelo_delivery_bus_t * bus,
    pomelo_delivery_fragment_meta_t * meta
) {
    assert(bus != NULL);
    assert(meta != NULL);

    pomelo_delivery_transporter_t * transporter = bus->endpoint->transporter;
    pomelo_delivery_context_t * context = transporter->context;
    pomelo_delivery_recv_command_t * command;
    pomelo_delivery_parcel_t * parcel;
    pomelo_unrolled_list_t * fragments;

    // Acquire a command from context pool
    command = pomelo_pool_acquire(transporter->recv_pool);
    if (!command) {
        // Failed to acquire new command
        return NULL;
    }

    // Acquire a parcel
    parcel = context->acquire_parcel(context);
    if (!parcel) {
        // Failed to acquire new parcel
        pomelo_delivery_recv_command_cleanup(command);
        return NULL;
    }
    command->parcel = parcel;
    fragments = parcel->fragments;

    // Reserve room for fragments
    int ret = pomelo_unrolled_list_resize(fragments, meta->total_fragments);
    if (ret < 0) {
        // Failed to reserve room for fragments
        pomelo_delivery_recv_command_cleanup(command);
        return NULL;
    }

    command->sequence = meta->parcel_sequence;
    command->bus = bus;
    switch (meta->type) {
        case POMELO_FRAGMENT_TYPE_DATA_RELIABLE:
            command->mode = POMELO_DELIVERY_MODE_RELIABLE;
            break;
        
        case POMELO_FRAGMENT_TYPE_DATA_SEQUENCED:
            command->mode = POMELO_DELIVERY_MODE_SEQUENCED;
            break;
        
        default:
            command->mode = POMELO_DELIVERY_MODE_UNRELIABLE;
    }

    return command;
}


pomelo_delivery_send_command_t * pomelo_delivery_send_command_prepare(
    pomelo_delivery_bus_t * bus,
    pomelo_delivery_parcel_t * parcel,
    pomelo_delivery_mode mode,
    uint64_t parcel_sequence
) {
    assert(bus != NULL);
    assert(parcel != NULL);

    pomelo_delivery_transporter_t * transporter = bus->endpoint->transporter;
    pomelo_delivery_send_command_t * command;

    // Acquire new command
    command = pomelo_pool_acquire(transporter->send_pool);
    if (!command) {
        // Failed to acquire new command
        return NULL;
    }

    // The parcel is referenced outside, so do not reference it here

    command->sequence = parcel_sequence;
    command->bus = bus;
    command->parcel = parcel;
    command->mode = mode;

    return command;
}


void pomelo_delivery_recv_command_cleanup(
    pomelo_delivery_recv_command_t * command
) {
    assert(command != NULL);
    pomelo_delivery_transporter_t * transporter =
        command->bus->endpoint->transporter;

    // Cleanup the timer
    if (command->expired_timer) {
        pomelo_platform_timer_stop(
            transporter->platform,
            command->expired_timer
        );
        command->expired_timer = NULL;
    }

    // Cleanup the parcel
    pomelo_delivery_parcel_t * parcel = command->parcel;
    if (parcel) {
        // Unlock the releasing flag
        pomelo_delivery_parcel_unref(parcel);
        command->parcel = NULL;
    }

    // Release the command
    pomelo_pool_release(transporter->recv_pool, command);
}


void pomelo_delivery_send_command_cleanup(
    pomelo_delivery_send_command_t * command
) {
    assert(command != NULL);

    pomelo_delivery_transporter_t * transporter =
        command->bus->endpoint->transporter;

    // Cleanup the parcel
    pomelo_delivery_parcel_t * parcel = command->parcel;
    if (parcel) {
        // Unlock releasing flag
        pomelo_delivery_parcel_unref(parcel);
        command->parcel = NULL;
    }

    // Cleanup the timer
    if (command->resend_timer) {
        pomelo_platform_timer_stop(
            transporter->platform,
            command->resend_timer
        );
        command->resend_timer = NULL;
    }

    // Release the command
    pomelo_pool_release(transporter->send_pool, command);
}


void pomelo_delivery_send_command_run(
    pomelo_delivery_send_command_t * command
) {
    assert(command != NULL);
    pomelo_delivery_endpoint_t * endpoint = command->bus->endpoint;

    if (command->mode == POMELO_DELIVERY_MODE_RELIABLE) {
        uint64_t rtt_mean = 0;
        pomelo_delivery_endpoint_rtt(endpoint, &rtt_mean, NULL);

        // Set a resending schedule
        uint64_t time_ms = POMELO_TIME_NS_TO_MS(POMELO_MAX(
            rtt_mean * POMELO_REL_MSG_RESENT_FACTOR_RTT,
            POMELO_REL_MSG_MIN_RESEND_TIME_NS
        ));

        command->resend_timer = pomelo_platform_timer_start(
            endpoint->transporter->platform,
            (pomelo_platform_timer_cb) pomelo_delivery_send_command_deliver,
            time_ms,
            time_ms, // With repeat
            command
        );

        if (!command->resend_timer) {
            // Failed to start timer, call the error callback
            pomelo_delivery_bus_on_send_command_error(command->bus, command);
            return;
        }
    }

    pomelo_delivery_send_command_deliver(command);

    if (command->mode != POMELO_DELIVERY_MODE_RELIABLE) {
        pomelo_delivery_bus_on_send_command_completed(command->bus, command);
    }
}


void pomelo_delivery_recv_command_run(
    pomelo_delivery_recv_command_t * command
) {
    assert(command != NULL);

    pomelo_delivery_endpoint_t * endpoint = command->bus->endpoint;
    pomelo_delivery_transporter_t * transporter = endpoint->transporter;

    if (
        command->mode != POMELO_DELIVERY_MODE_RELIABLE &&
        command->parcel->fragments->size > 1
    ) {
        uint64_t rtt_mean = 0;
        pomelo_delivery_endpoint_rtt(endpoint, &rtt_mean, NULL);

        // Sequenced & unreliable
        // Set a timeout
        uint64_t time_ms = POMELO_TIME_NS_TO_MS(POMELO_MIN(
            rtt_mean * POMELO_UNREL_MSG_EXPIRED_FACTOR_RTT,
            POMELO_UNREL_MSG_MAX_ALIVE_TIME_NS
        ));

        command->expired_timer = pomelo_platform_timer_start(
            transporter->platform,
            (pomelo_platform_timer_cb) pomelo_delivery_recv_command_on_expired,
            time_ms,
            0, // No repeat
            command
        );
    }
}


void pomelo_delivery_send_command_receive_ack(
    pomelo_delivery_send_command_t * command,
    pomelo_delivery_fragment_meta_t * meta
) {
    assert(command != NULL);
    assert(meta != NULL);

    pomelo_unrolled_list_t * fragments = command->parcel->fragments;
    pomelo_delivery_fragment_t * fragment = NULL;

    pomelo_unrolled_list_get(fragments, meta->fragment_index, &fragment);
    if (fragment == NULL) return; // The fragment has not arrived

    // The command must be reliable
    if (command->mode != POMELO_DELIVERY_MODE_RELIABLE) return;

    fragment->acked = 1;
    if ((++command->acked_counter) == command->parcel->fragments->size) {
        // Enough acked sending command
        pomelo_delivery_bus_on_send_command_completed(command->bus, command);
    }
}


int pomelo_delivery_recv_command_receive_fragment(
    pomelo_delivery_recv_command_t * command,
    pomelo_delivery_fragment_t * fragment
) {
    assert(command != NULL);
    assert(fragment != NULL);

    pomelo_delivery_fragment_t * frag = NULL;
    pomelo_unrolled_list_t * fragments = command->parcel->fragments;

    pomelo_unrolled_list_get(fragments, fragment->index, &frag);
    if (frag != NULL) return -1; // The fragment has arrived before

    int ret = pomelo_unrolled_list_set(fragments, fragment->index, fragment);
    if (ret < 0) return ret; // Failed to set the fragment
    
    if ((++command->received_counter) == command->parcel->fragments->size) {
        // Receives enough fragments, call the callback
        pomelo_delivery_bus_on_recv_command_completed(command->bus, command);
    }

    return 0;
}


void pomelo_delivery_recv_command_on_expired(
    pomelo_delivery_recv_command_t * command
) {
    assert(command != NULL);

    // Pass recv command to bus because it stores the command.
    pomelo_delivery_bus_on_recv_command_expired(command->bus, command);
}


void pomelo_delivery_send_command_deliver(
    pomelo_delivery_send_command_t * command
) {
    assert(command != NULL);
    pomelo_delivery_endpoint_t * endpoint = command->bus->endpoint;

    pomelo_unrolled_list_iterator_t it;
    pomelo_unrolled_list_begin(command->parcel->fragments, &it);
    pomelo_delivery_fragment_t * fragment;

    while (pomelo_unrolled_list_iterator_next(&it, &fragment)) {
        // Process sending the fragment here
        if (fragment->acked) continue;

        // For reliable parcel, all fragments will be never acked.
        // So this checking works for both modes.
        int ret = pomelo_delivery_endpoint_send(
            endpoint,
            fragment->buffer,
            /* offset = */ 0,
            fragment->payload.capacity
        );
        if (ret < 0) {
            // Mark this command as error sending
            pomelo_delivery_bus_on_send_command_error(command->bus, command);
            // Stop continue sending
            break;
        }
    }
}
