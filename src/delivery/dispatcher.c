#include <assert.h>
#include <string.h>
#include "crypto/checksum.h"
#include "utils/macro.h"
#include "dispatcher.h"
#include "bus.h"
#include "endpoint.h"
#include "parcel.h"
#include "context.h"
#include "sender.h"


/// The minimum parcel resending schedule. The parcel will be resent after
/// a certain time but not less than this value.
#define POMELO_REL_MSG_MIN_RESEND_TIME_NS 10000000ULL // 10ms

/// The maximum parcel resending schedule. The parcel will be resent after
/// a certain time but not greater than this value.
#define POMELO_REL_MSG_MAX_RESEND_TIME_NS 100000000ULL // 100ms

/// The factor of resending time for reliable parcel:
/// resend = factor * rtt
#define POMELO_REL_MSG_RESENT_FACTOR_RTT 1

/// The initial capacity of the fragments array
#define POMELO_DELIVERY_DISPATCHER_FRAGMENTS_INIT_CAPACITY 16


/// @brief The tasks of the dispatcher
static pomelo_pipeline_entry_fn dispatcher_tasks[] = {
    (pomelo_pipeline_entry_fn) pomelo_delivery_dispatcher_dispatch,
    (pomelo_pipeline_entry_fn) pomelo_delivery_dispatcher_complete,
};

/* -------------------------------------------------------------------------- */
/*                             Sending Command                                */
/* -------------------------------------------------------------------------- */

int pomelo_delivery_dispatcher_on_alloc(
    pomelo_delivery_dispatcher_t * dispatcher,
    pomelo_delivery_context_t * context
) {
    assert(dispatcher != NULL);
    assert(context != NULL);

    pomelo_array_options_t array_options = {
        .allocator = context->allocator,
        .element_size = sizeof(pomelo_delivery_fragment_t),
        .initial_capacity = POMELO_DELIVERY_DISPATCHER_FRAGMENTS_INIT_CAPACITY
    };
    dispatcher->context = context;
    dispatcher->fragments = pomelo_array_create(&array_options);
    if (!dispatcher->fragments) return -1; // Failed to create new array

    return 0;
}


void pomelo_delivery_dispatcher_on_free(
    pomelo_delivery_dispatcher_t * dispatcher
) {
    assert(dispatcher != NULL);
    dispatcher->context = NULL;

    if (dispatcher->fragments) {
        pomelo_array_destroy(dispatcher->fragments);
        dispatcher->fragments = NULL;
    }
}


int pomelo_delivery_dispatcher_init(
    pomelo_delivery_dispatcher_t * dispatcher,
    pomelo_delivery_dispatcher_info_t * info
) {
    assert(dispatcher != NULL);
    assert(info != NULL);

    pomelo_delivery_bus_t * bus = info->bus;
    pomelo_delivery_parcel_t * parcel = info->parcel;
    pomelo_delivery_endpoint_t * endpoint = bus->endpoint;
    pomelo_delivery_context_t * context = bus->context;

    // Initialize the pipeline
    pomelo_pipeline_options_t pipeline_options = {
        .tasks = dispatcher_tasks,
        .task_count = sizeof(dispatcher_tasks) / sizeof(dispatcher_tasks[0]),
        .callback_data = dispatcher,
        .sequencer = endpoint->sequencer
    };
    int ret = pomelo_pipeline_init(&dispatcher->pipeline, &pipeline_options);
    if (ret < 0) return ret;

    dispatcher->platform = endpoint->platform;
    dispatcher->bus = bus;
    dispatcher->endpoint = bus->endpoint;
    dispatcher->acked_counter = 0;
    dispatcher->mode = info->mode;
    dispatcher->sequence = info->sequence;
    dispatcher->sequencer = endpoint->sequencer;
    dispatcher->flags = 0;

    dispatcher->checksum = info->sender->checksum;
    if (dispatcher->checksum) {
        pomelo_buffer_ref(dispatcher->checksum);
    }

    dispatcher->sender = info->sender;
    dispatcher->sender_entry = NULL;

    // Initialize the resend task
    pomelo_sequencer_task_init(
        &dispatcher->resend_task,
        (pomelo_sequencer_callback) pomelo_delivery_dispatcher_resend,
        dispatcher
    );

    // Check the checksum mode
    pomelo_array_t * chunks = parcel->chunks;
    if (chunks->size >= 2) {
        pomelo_buffer_view_t * last_chunk =
            pomelo_array_get_ptr(chunks, chunks->size - 1);
        assert(last_chunk != NULL);

        size_t buffer_capacity = (
            last_chunk->buffer->capacity - last_chunk->offset
        );
        size_t capacity = POMELO_MIN(
            context->fragment_content_capacity,
            buffer_capacity
        );
        size_t remain = capacity - last_chunk->length;
        dispatcher->checksum_mode = (remain >= POMELO_CRYPTO_CHECKSUM_BYTES)
            ? POMELO_DELIVERY_CHECKSUM_EMBEDDED
            : POMELO_DELIVERY_CHECKSUM_EXTRA;
    } else {
        // No checksum
        dispatcher->checksum_mode = POMELO_DELIVERY_CHECKSUM_NONE;
    }

    // Prepare the array of fragments
    size_t nfragments = chunks->size;
    if (dispatcher->checksum_mode == POMELO_DELIVERY_CHECKSUM_EXTRA) {
        nfragments += 1; // An extra fragment for checksum
    }

    // Initialize the fragments
    pomelo_array_t * fragments = dispatcher->fragments;
    ret = pomelo_array_resize(fragments, nfragments);
    if (ret < 0) return ret; // Failed to resize the array
    for (size_t i = 0; i < nfragments; i++) {
        pomelo_delivery_fragment_t * fragment =
            pomelo_array_get_ptr(fragments, i);
        assert(fragment != NULL);
        pomelo_delivery_fragment_init(fragment);
    }

    size_t nchunks = chunks->size;
    for (size_t i = 0; i < nchunks; i++) {
        pomelo_buffer_view_t * chunk = pomelo_array_get_ptr(chunks, i);
        assert(chunk != NULL);

        pomelo_delivery_fragment_t * fragment =
            pomelo_array_get_ptr(fragments, i);
        assert(fragment != NULL);

        pomelo_delivery_fragment_attach_content(fragment, chunk);
        fragment->acked = false;
    }

    if (dispatcher->checksum_mode == POMELO_DELIVERY_CHECKSUM_EXTRA) {
        // Update the last fragment
        pomelo_delivery_fragment_t * fragment =
            pomelo_array_get_ptr(fragments, fragments->size - 1);
        assert(fragment != NULL);

        // Acquire the checksum buffer
        pomelo_delivery_fragment_attach_buffer(fragment, dispatcher->checksum);
        fragment->content.length = POMELO_CRYPTO_CHECKSUM_BYTES;
        fragment->acked = false;
    }


    // Add the sending command to the sub-commands list
    dispatcher->sender_entry =
        pomelo_list_push_back(dispatcher->sender->dispatchers, dispatcher);
    if (!dispatcher->sender_entry) {
        return -1; // Failed to add the dispatcher to the list
    }


    // Add sender to the bus
    if (!pomelo_list_push_back(bus->pending_dispatchers, dispatcher)) {
        return -1; // Failed to add the dispatcher to the bus
    }

    return 0;
}


void pomelo_delivery_dispatcher_cleanup(
    pomelo_delivery_dispatcher_t * dispatcher
) {
    assert(dispatcher != NULL);
    pomelo_pipeline_cleanup(&dispatcher->pipeline);

    // Cleanup the fragments
    pomelo_array_t * fragments = dispatcher->fragments;
    size_t nfragments = fragments->size;
    for (size_t i = 0; i < nfragments; i++) {
        pomelo_delivery_fragment_t * fragment =
            pomelo_array_get_ptr(fragments, i);
        assert(fragment != NULL);
        pomelo_delivery_fragment_cleanup(fragment);
    }
    pomelo_array_clear(fragments);

    // Cleanup the checksum
    if (dispatcher->checksum) {
        pomelo_buffer_unref(dispatcher->checksum);
        dispatcher->checksum = NULL;
    }

    if (dispatcher->sender_entry) {
        pomelo_list_remove(
            dispatcher->sender->dispatchers,
            dispatcher->sender_entry
        );
        dispatcher->sender_entry = NULL;
    }

    // Cleanup the resend timer
    pomelo_platform_timer_stop(
        dispatcher->platform,
        &dispatcher->resend_timer
    );
}


void pomelo_delivery_dispatcher_cancel(
    pomelo_delivery_dispatcher_t * dispatcher
) {
    assert(dispatcher != NULL);
    if (dispatcher->flags & POMELO_DELIVERY_DISPATCHER_FLAG_CANCELED) return;
    dispatcher->flags |= POMELO_DELIVERY_DISPATCHER_FLAG_CANCELED;

    // Cancel resend timer
    pomelo_platform_timer_stop(
        dispatcher->platform,
        &dispatcher->resend_timer
    );

    pomelo_pipeline_finish(&dispatcher->pipeline);
}


void pomelo_delivery_dispatcher_submit(
    pomelo_delivery_dispatcher_t * dispatcher
) {
    assert(dispatcher != NULL);
    // Start the pipeline
    pomelo_pipeline_start(&dispatcher->pipeline);
}


/// @brief Handle the resend timer triggered event
static void on_resend_timer_triggered(
    pomelo_delivery_dispatcher_t * dispatcher
) {
    assert(dispatcher != NULL);
    pomelo_sequencer_submit(dispatcher->sequencer, &dispatcher->resend_task);
}


void pomelo_delivery_dispatcher_dispatch(
    pomelo_delivery_dispatcher_t * dispatcher
) {
    assert(dispatcher != NULL);

    pomelo_delivery_bus_t * bus = dispatcher->bus;
    pomelo_delivery_endpoint_t * endpoint = bus->endpoint;

    // Send the parcel first time
    int ret = pomelo_delivery_dispatcher_send(dispatcher);
    if (ret < 0) {
        // Failed to dispatch the parcel
        dispatcher->flags |= POMELO_DELIVERY_DISPATCHER_FLAG_FAILED;
        pomelo_pipeline_finish(&dispatcher->pipeline);
        return;
    }

    if (dispatcher->mode != POMELO_DELIVERY_MODE_RELIABLE) {
        // Other modes than reliable do not need to resend, next stage
        pomelo_pipeline_next(&dispatcher->pipeline);
        return;
    }

    // Setup resent timer
    uint64_t rtt_mean = 0;
    pomelo_delivery_endpoint_rtt(endpoint, &rtt_mean, NULL);

    // Set a resending schedule
    uint64_t time_ns = rtt_mean * POMELO_REL_MSG_RESENT_FACTOR_RTT;
    if (time_ns < POMELO_REL_MSG_MIN_RESEND_TIME_NS) {
        time_ns = POMELO_REL_MSG_MIN_RESEND_TIME_NS;
    } else if (time_ns > POMELO_REL_MSG_MAX_RESEND_TIME_NS) {
        time_ns = POMELO_REL_MSG_MAX_RESEND_TIME_NS;
    }

    uint64_t time_ms = POMELO_TIME_NS_TO_MS(time_ns);

    ret = pomelo_platform_timer_start(
        dispatcher->platform,
        (pomelo_platform_timer_entry) on_resend_timer_triggered,
        time_ms,
        time_ms, // With repeat
        dispatcher,
        &dispatcher->resend_timer
    );
    if (ret < 0) {
        // Failed to start timer
        dispatcher->flags |= POMELO_DELIVERY_DISPATCHER_FLAG_FAILED;
        pomelo_pipeline_finish(&dispatcher->pipeline);
        return;
    }
}


int pomelo_delivery_dispatcher_send(pomelo_delivery_dispatcher_t * dispatcher) {
    assert(dispatcher != NULL);
    pomelo_buffer_view_t views[3];

    pomelo_delivery_endpoint_t * endpoint = dispatcher->endpoint;
    pomelo_buffer_context_t * buffer_context =
        dispatcher->context->buffer_context;

    // Get the last fragment
    pomelo_array_t * fragments = dispatcher->fragments;
    size_t nfragments = fragments->size;
    size_t last_fragment_index = nfragments - 1;

    // Build the meta of the fragments
    pomelo_delivery_fragment_meta_t meta;
    meta.bus_id = dispatcher->bus->id;
    meta.sequence = dispatcher->sequence;
    meta.type = pomelo_delivery_fragment_type_from_mode(dispatcher->mode);
    meta.last_index = nfragments - 1;

    for (size_t i = 0; i < nfragments; i++) {
        pomelo_delivery_fragment_t * fragment =
            pomelo_array_get_ptr(fragments, i);
        assert(fragment != NULL);
        if (fragment->acked) continue; // Ignore acked fragments

        // Acquire new buffer for the meta
        pomelo_buffer_t * buffer_meta =
            pomelo_buffer_context_acquire(buffer_context);
        if (!buffer_meta) return -1; // Failed to acquire buffer
        views[0].buffer = buffer_meta;
        views[0].offset = 0;
        views[0].length = 0;

        // Build the meta
        meta.fragment_index = i;
        int ret = pomelo_delivery_fragment_meta_encode(&meta, &views[0]);
        if (ret < 0) {
            pomelo_buffer_unref(buffer_meta);
            return ret; // Failed to build the meta
        }

        views[1] = fragment->content;
        size_t view_count = 2;

        if (
            i == last_fragment_index &&
            dispatcher->checksum_mode == POMELO_DELIVERY_CHECKSUM_EMBEDDED
        ) {
            // Send checksum along with the last fragment
            pomelo_buffer_view_t * view_checksum = &views[2];
            view_checksum->buffer = dispatcher->checksum;
            view_checksum->offset = 0;
            view_checksum->length = POMELO_CRYPTO_CHECKSUM_BYTES;
            view_count = 3;
        }

        ret = pomelo_delivery_endpoint_send(endpoint, views, view_count);
        if (ret < 0) {
            pomelo_buffer_unref(buffer_meta); // Unref the meta buffer
            return ret; // Failed to send the fragment
        }

        // Finally, unref the meta buffer
        pomelo_buffer_unref(buffer_meta);
    }

    return 0;
}


void pomelo_delivery_dispatcher_resend(
    pomelo_delivery_dispatcher_t * dispatcher
) {
    assert(dispatcher != NULL);

    int ret = pomelo_delivery_dispatcher_send(dispatcher);
    if (ret < 0) {
        dispatcher->flags |= POMELO_DELIVERY_DISPATCHER_FLAG_FAILED;
        pomelo_pipeline_finish(&dispatcher->pipeline);
    }
}


void pomelo_delivery_dispatcher_recv_ack(
    pomelo_delivery_dispatcher_t * dispatcher,
    pomelo_delivery_fragment_meta_t * meta
) {
    assert(dispatcher != NULL);
    assert(meta != NULL);

    // The command must be reliable
    if (dispatcher->mode != POMELO_DELIVERY_MODE_RELIABLE) return;

    pomelo_array_t * fragments = dispatcher->fragments;
    pomelo_delivery_fragment_t * fragment =
        pomelo_array_get_ptr(fragments, meta->fragment_index);
    if (!fragment) return; // The fragment does not exist
    if (fragment->acked) return; // The fragment has already been acked

    // Mark the fragment as acked
    fragment->acked = true;
    dispatcher->acked_counter++;

    if (dispatcher->acked_counter < fragments->size) {
        return; // Not enough acked fragments
    }

    // Enough acked fragments, next stage.
    pomelo_pipeline_next(&dispatcher->pipeline);
}


void pomelo_delivery_dispatcher_complete(
    pomelo_delivery_dispatcher_t * dispatcher
) {
    assert(dispatcher != NULL);

    pomelo_delivery_sender_t * sender = dispatcher->sender;
    pomelo_delivery_context_t * context = dispatcher->context;

    if (dispatcher->flags & POMELO_DELIVERY_DISPATCHER_FLAG_CANCELED) {
        // Submit result        
        pomelo_delivery_sender_on_dispatcher_result(sender, dispatcher);
        
        // The command has been canceled, release itself
        pomelo_pool_release(context->dispatcher_pool, dispatcher);
        return;
    }

    // Remove the dispatcher from the sender's sub-commands list
    assert(dispatcher->sender_entry != NULL);
    pomelo_list_remove(sender->dispatchers, dispatcher->sender_entry);
    dispatcher->sender_entry = NULL;

    // Dispatch the complete event to send batch command
    pomelo_delivery_sender_on_dispatcher_result(sender, dispatcher);

    // Handle the completion of the dispatcher
    pomelo_delivery_bus_on_dispatcher_completed(dispatcher->bus, dispatcher);

    // Finally, release the dispatcher
    pomelo_pool_release(context->dispatcher_pool, dispatcher);
}
