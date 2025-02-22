#include <assert.h>
#include <string.h>
#include "utils/macro.h"
#include "bus.h"
#include "endpoint.h"
#include "commands.h"
#include "parcel.h"


/* -------------------------------------------------------------------------- */
/*                               Private APIs                                 */
/* -------------------------------------------------------------------------- */


int pomelo_delivery_bus_init(
    pomelo_delivery_bus_t * bus,
    pomelo_delivery_endpoint_t * endpoint
) {
    assert(bus != NULL);
    assert(endpoint != NULL);

    memset(bus, 0, sizeof(pomelo_delivery_bus_t));

    pomelo_allocator_t * allocator = endpoint->transporter->allocator;
    bus->endpoint = endpoint;

    // Create pending send parcels list
    pomelo_list_options_t list_options;
    pomelo_list_options_init(&list_options);
    list_options.allocator = allocator;
    list_options.element_size = sizeof(pomelo_delivery_send_command_t *);

    bus->pending_send_commands = pomelo_list_create(&list_options);
    if (!bus->pending_send_commands) {
        pomelo_delivery_bus_finalize(bus);
        return -1;
    }

    // Create incomplete receiving parcels map
    pomelo_map_options_t map_options;
    pomelo_map_options_init(&map_options);
    map_options.allocator = allocator;
    map_options.key_size = sizeof(uint64_t);
    map_options.value_size = sizeof(pomelo_delivery_recv_command_t *);
    bus->incomplete_recv_commands = pomelo_map_create(&map_options);
    if (!bus->incomplete_recv_commands) {
        pomelo_delivery_bus_finalize(bus);
        return -1;
    }

    return 0;
}


void pomelo_delivery_bus_reset(pomelo_delivery_bus_t * bus) {
    assert(bus != NULL);

    // Cleanup sending commands
    if (bus->pending_send_commands) {
        pomelo_delivery_send_command_t * send_command = NULL;
        pomelo_list_ptr_for(bus->pending_send_commands, send_command, {
            pomelo_delivery_send_command_cleanup(send_command);
        });

        pomelo_list_clear(bus->pending_send_commands);
    }

    // Cleanup receiving commands
    if (bus->incomplete_recv_commands) {
        pomelo_map_iterator_t it;
        pomelo_map_entry_t entry;

        pomelo_map_iterate(bus->incomplete_recv_commands, &it);
        pomelo_delivery_recv_command_t * recv_command = NULL;
        while (pomelo_map_iterator_next(&it, &entry) == 0) { // OK
            recv_command = pomelo_map_entry_value_ptr(&entry);

            // Release the recving command
            pomelo_delivery_recv_command_cleanup(recv_command);
        }

        pomelo_map_clear(bus->incomplete_recv_commands);
    }

    // Cleanup current reliable sending command
    if (bus->incomplete_reliable_send_command) {
        pomelo_delivery_send_command_cleanup(
            bus->incomplete_reliable_send_command
        );
        bus->incomplete_reliable_send_command = NULL;
    }

    bus->incomplete_reliable_recv_command = NULL;
    
    // Cleanup values
    bus->most_recent_recv_reliable_sequence = 0;
    bus->parcel_sequence = 0;
    bus->most_recent_recv_sequenced_sequence = 0;
}


void pomelo_delivery_bus_finalize(pomelo_delivery_bus_t * bus) {
    assert(bus != NULL);
    
    // Cleanup all commands
    pomelo_delivery_bus_reset(bus);

    if (bus->pending_send_commands) {
        pomelo_list_destroy(bus->pending_send_commands);
        bus->pending_send_commands = NULL;
    }

    if (bus->incomplete_recv_commands) {
        pomelo_map_destroy(bus->incomplete_recv_commands);
        bus->incomplete_recv_commands = NULL;
    }
}


int pomelo_delivery_bus_on_received_payload(
    pomelo_delivery_bus_t * bus,
    pomelo_delivery_fragment_meta_t * meta,
    pomelo_buffer_t * buffer,
    pomelo_payload_t * payload
) {
    assert(bus != NULL);
    assert(meta != NULL);
    assert(buffer != NULL);
    assert(payload != NULL);

    if (meta->type == POMELO_FRAGMENT_TYPE_ACK) {
        return pomelo_delivery_bus_on_received_fragment_ack(bus, meta);
    }

    return pomelo_delivery_bus_on_received_fragment_data(
        bus, meta, buffer, payload
    );
}


int pomelo_delivery_bus_on_received_fragment_data(
    pomelo_delivery_bus_t * bus,
    pomelo_delivery_fragment_meta_t * meta,
    pomelo_buffer_t * buffer,
    pomelo_payload_t * payload
) {
    assert(bus != NULL);
    assert(meta != NULL);
    assert(buffer != NULL);
    assert(payload != NULL);

    pomelo_map_t * commands = bus->incomplete_recv_commands;
    pomelo_delivery_recv_command_t * incomplete_reliable_recv_command = 
        bus->incomplete_reliable_recv_command;

    // The parcel sequence
    uint64_t sequence = meta->parcel_sequence;

    if (meta->type == POMELO_FRAGMENT_TYPE_DATA_RELIABLE) {
        if (incomplete_reliable_recv_command) {
            // There's a incomplete reliable command
            if (sequence != incomplete_reliable_recv_command->sequence) {
                // Mismatch reliable parcel sequence
                return -1;
            }
        } else {
            // There's no incomplete reliable command
            if (bus->most_recent_recv_reliable_sequence >= sequence) {
                // The fragment is out of date, just reply with ACK
                pomelo_delivery_bus_reply_ack(bus, meta);
                return -1;
            }
        }
    }

    pomelo_delivery_recv_command_t * command = NULL;
    pomelo_map_get(commands, sequence, &command);
    if (!command) {
        // If there's no command in map, prepare new one
        command = pomelo_delivery_recv_command_prepare(bus, meta);
        if (!command) {
            // Cannot create new command
            return -1;
        }

        int ret = pomelo_map_set(commands, sequence, command);
        if (ret < 0) {
            // Cannot set command to map
            pomelo_delivery_recv_command_cleanup(command);
            return ret;
        }

        // Set the pending reliable command
        if (command->mode == POMELO_DELIVERY_MODE_RELIABLE) {
            bus->incomplete_reliable_recv_command = command;
            bus->most_recent_recv_reliable_sequence = sequence;
        }

        // Then run it
        pomelo_delivery_recv_command_run(command);
    }

    pomelo_delivery_context_t * context = bus->endpoint->transporter->context;

    // Acquire new fragment and attach buffer to fragment
    pomelo_delivery_fragment_t * fragment =
        context->acquire_fragment(context, buffer);
    if (!fragment) {
        // Failed to acquire new fragment
        return -1;
    }

    // The buffer actually is assigned to payload before.
    fragment->payload = *payload;
    fragment->index = meta->fragment_index;
    fragment->acked = 0;

    // Reply ack
    int ret;
    if (command->mode == POMELO_DELIVERY_MODE_RELIABLE) {
        ret = pomelo_delivery_bus_reply_ack(bus, meta);
        if (ret < 0) {
            return ret;
        }
    }

    ret = pomelo_delivery_recv_command_receive_fragment(command, fragment);
    if (ret < 0) {
        // Failed to receive fragment, release the fragment and its associated
        // buffer.
        context->release_fragment(context, fragment);
        return ret;
    }

    // If the command receives enough fragments, the next callback will be:
    // => pomelo_delivery_bus_on_recv_command_completed
    return 0;
}


int pomelo_delivery_bus_on_received_fragment_ack(
    pomelo_delivery_bus_t * bus,
    pomelo_delivery_fragment_meta_t * meta
) {
    assert(bus != NULL);
    assert(meta != NULL);

    pomelo_delivery_send_command_t * command =
        bus->incomplete_reliable_send_command;

    if (!command || command->sequence != meta->parcel_sequence) {
        // Failed to get command
        return -1;
    }

    // Process ack
    pomelo_delivery_send_command_receive_ack(command, meta);
    return 0;
}


void pomelo_delivery_bus_on_send_command_completed(
    pomelo_delivery_bus_t * bus,
    pomelo_delivery_send_command_t * command
) {
    assert(bus != NULL);
    assert(command != NULL);

    pomelo_delivery_send_command_cleanup(command);

    if (command == bus->incomplete_reliable_send_command) {
        // Clear the sending command and continue processing
        bus->incomplete_reliable_send_command = NULL;
        pomelo_delivery_bus_process_sending(bus);
    }
}


void pomelo_delivery_bus_on_recv_command_completed(
    pomelo_delivery_bus_t * bus,
    pomelo_delivery_recv_command_t * command
) {
    assert(bus != NULL);
    assert(command != NULL);

    pomelo_delivery_bus_postprocess_recv_parcel(bus, command);
    // => pomelo_delivery_bus_postprocess_recv_parcel_done
}



void pomelo_delivery_bus_postprocess_recv_parcel(
    pomelo_delivery_bus_t * bus,
    pomelo_delivery_recv_command_t * command
) {
    assert(bus != NULL);
    assert(command != NULL);

    if (command->parcel->fragments->size < 2) {
        // No checksum to compute
        pomelo_delivery_bus_postprocess_recv_parcel_done(bus, command, 0);
        return;
    }
    
    pomelo_delivery_bus_validate_parcel_checksum(bus, command);
}


void pomelo_delivery_bus_validate_parcel_checksum(
    pomelo_delivery_bus_t * bus,
    pomelo_delivery_recv_command_t * command
) {
    assert(bus != NULL);
    assert(command != NULL);

    pomelo_delivery_transporter_t * transporter =
        bus->endpoint->transporter;
    pomelo_delivery_checksum_command_t * checksum_command =
        pomelo_pool_acquire(transporter->checksum_pool);
    if (!checksum_command) {
        // Failed to acquire new checksum command
        pomelo_delivery_bus_validate_parcel_checksum_done(
            bus, command, checksum_command
        );
        return;
    }

    pomelo_delivery_checksum_validating_data_t * validating =
        &checksum_command->specific.validating;
    pomelo_delivery_parcel_t * parcel = command->parcel;
    int ret = pomelo_delivery_parcel_slice_checksum(
        parcel,
        validating->embedded_checksum
    );
    if (ret < 0) {
        // Failed to extract checksum from parcel
        checksum_command->result = -1;
        pomelo_delivery_bus_validate_parcel_checksum_done(
            bus, command, checksum_command
        );
        return;
    }

    checksum_command->callback_mode = 
        POMELO_DELIVERY_CHECKSUM_CALLBACK_VALIDATE;
    checksum_command->bus = bus;
    checksum_command->parcel = parcel;

    // Store specific fields for validating
    validating->recv_command = command;

    ret = pomelo_platform_submit_worker_task(
        transporter->platform,
        transporter->task_group,
        (pomelo_platform_task_cb) pomelo_delivery_checksum_command_entry,
        (pomelo_platform_task_done_cb) pomelo_delivery_checksum_command_done,
        checksum_command
    );

    if (ret < 0) {
        checksum_command->result = -1;
        pomelo_delivery_bus_validate_parcel_checksum_done(
            bus, command, checksum_command
        );
    }

    // => pomelo_delivery_bus_validate_parcel_checksum_done
}


void pomelo_delivery_bus_validate_parcel_checksum_done(
    pomelo_delivery_bus_t * bus,
    pomelo_delivery_recv_command_t * command,
    pomelo_delivery_checksum_command_t * checksum_command
) {
    assert(bus != NULL);
    assert(command != NULL);

    if (!checksum_command) {
        // Failed to acquire new command
        pomelo_delivery_bus_postprocess_recv_parcel_done(
            bus, command, -1
        );
        return;
    }

    int ret = memcmp(
        checksum_command->checksum,
        checksum_command->specific.validating.embedded_checksum,
        POMELO_CODEC_CHECKSUM_BYTES
    );

    // Release the command
    pomelo_pool_release(
        bus->endpoint->transporter->checksum_pool,
        checksum_command
    );
    
    // Call the callback
    pomelo_delivery_bus_postprocess_recv_parcel_done(bus, command, ret);
}


void pomelo_delivery_bus_postprocess_recv_parcel_done(
    pomelo_delivery_bus_t * bus,
    pomelo_delivery_recv_command_t * command,
    int result
) {
    assert(bus != NULL);
    assert(command != NULL);

    if (result < 0) {
        // Checksum failed
        pomelo_delivery_bus_remove_recv_command(bus, command);
        return;
    }

    pomelo_delivery_parcel_t * parcel = command->parcel;
    if (command->mode == POMELO_DELIVERY_MODE_SEQUENCED) {
        // For sequenced parcel
        if (command->sequence > bus->most_recent_recv_sequenced_sequence) {
            // Only accept the higher sequence
            bus->most_recent_recv_sequenced_sequence = command->sequence;
            pomelo_delivery_bus_on_received(bus, parcel);
        }
    } else {
        // For reliable and unreliable
        pomelo_delivery_bus_on_received(bus, parcel);
    }

    // Release the command after receiving the parcel.
    // This will release the parcel as well.
    pomelo_delivery_bus_remove_recv_command(bus, command);
}


void pomelo_delivery_bus_process_sending(pomelo_delivery_bus_t * bus) {
    assert(bus != NULL);

    pomelo_list_t * commands = bus->pending_send_commands;
    pomelo_delivery_send_command_t * command;

    while (
        !bus->incomplete_reliable_send_command &&
        pomelo_list_pop_front(commands, &command) == 0)
    {
        if (command->mode == POMELO_DELIVERY_MODE_RELIABLE) {
            // This will block the bus
            bus->incomplete_reliable_send_command = command;
        }

        pomelo_delivery_send_command_run(command);
    }
}


int pomelo_delivery_bus_update_parcel_meta(
    pomelo_delivery_bus_t * bus,
    pomelo_delivery_send_command_t * command
) {
    assert(bus != NULL);
    assert(command != NULL);

    pomelo_delivery_fragment_t * fragment;
    pomelo_unrolled_list_t * fragments = command->parcel->fragments;
    pomelo_unrolled_list_iterator_t it;

    pomelo_unrolled_list_begin(fragments, &it);

    pomelo_delivery_fragment_meta_t meta;
    meta.bus_index = bus->index;
    meta.parcel_sequence = command->sequence;
    meta.total_fragments = fragments->size;
    switch (command->mode) {
        case POMELO_DELIVERY_MODE_RELIABLE:
            meta.type = POMELO_FRAGMENT_TYPE_DATA_RELIABLE;
            break;
        
        case POMELO_DELIVERY_MODE_SEQUENCED:
            meta.type = POMELO_FRAGMENT_TYPE_DATA_SEQUENCED;
            break;

        default:
            meta.type = POMELO_FRAGMENT_TYPE_DATA_UNRELIABLE;
    }
    
    while (pomelo_unrolled_list_iterator_next(&it, &fragment)) {
        meta.fragment_index = fragment->index;
        pomelo_payload_t * payload = &fragment->payload;

        int ret = pomelo_delivery_fragment_meta_encode(&meta, payload);
        if (ret < 0) return ret;
    }

    return 0;
}


void pomelo_delivery_bus_on_recv_command_expired(
    pomelo_delivery_bus_t * bus,
    pomelo_delivery_recv_command_t * command
) {
    assert(bus != NULL);
    assert(command != NULL);

    // Just like completed but without callback
    pomelo_delivery_bus_remove_recv_command(bus, command);
}


void pomelo_delivery_bus_on_send_command_error(
    pomelo_delivery_bus_t * bus,
    pomelo_delivery_send_command_t * command
) {
    assert(bus != NULL);
    assert(command != NULL);

    // Maybe there's another way better to catch this exception

    // Cleanup the command and continue processing
    pomelo_delivery_send_command_cleanup(command);

    if (command == bus->incomplete_reliable_send_command) {
        bus->incomplete_reliable_send_command = NULL;
        pomelo_delivery_bus_process_sending(bus);
    }
}


void pomelo_delivery_bus_remove_recv_command(
    pomelo_delivery_bus_t * bus,
    pomelo_delivery_recv_command_t * command
) {
    assert(bus != NULL);
    assert(command != NULL);

    int ret;

    // Remove command from map
    ret = pomelo_map_del(bus->incomplete_recv_commands, command->sequence);
    if (ret != 0) {
        // The command might be cleaned up before
        return;
    }

    // Cleanup the command
    pomelo_delivery_recv_command_cleanup(command);

    // Cleanup the recv command
    if (command == bus->incomplete_reliable_recv_command) {
        bus->incomplete_reliable_recv_command = NULL;
    }
}


/* -------------------------------------------------------------------------- */
/*                               Public APIs                                  */
/* -------------------------------------------------------------------------- */

int pomelo_delivery_bus_send(
    pomelo_delivery_bus_t * bus,
    pomelo_delivery_parcel_t * parcel,
    pomelo_delivery_mode mode
) {
    assert(bus != NULL);
    assert(parcel != NULL);

    if (parcel->fragments->size == 0) {
        // Empty data
        return -1;
    }

    // Temporary ref the parcel to ensure it will not be released.
    pomelo_delivery_parcel_ref(parcel);

    pomelo_delivery_bus_preprocess_send_parcel(bus, parcel, mode);
    // => pomelo_delivery_bus_preprocess_send_parcel_done

    return 0;
}


void pomelo_delivery_bus_preprocess_send_parcel(
    pomelo_delivery_bus_t * bus,
    pomelo_delivery_parcel_t * parcel,
    pomelo_delivery_mode mode
) {
    assert(bus != NULL);
    assert(parcel != NULL);

    if (parcel->fragments->size < 2) {
        // No checksum is required
        pomelo_delivery_bus_preprocess_send_parcel_done(
            bus, parcel, mode, /* result = */ 0 
        );
        return;
    }

    // Append checksum
    pomelo_delivery_bus_update_parcel_checksum(bus, parcel, mode);
    // => pomelo_delivery_bus_update_parcel_checksum_done
}


void pomelo_delivery_bus_update_parcel_checksum(
    pomelo_delivery_bus_t * bus,
    pomelo_delivery_parcel_t * parcel,
    pomelo_delivery_mode mode
) {
    assert(bus != NULL);
    assert(parcel != NULL);

    // Update last fragment capacity before calculating the checksum
    int ret;

    pomelo_delivery_transporter_t * transporter = bus->endpoint->transporter;

    pomelo_delivery_checksum_command_t * command =
        pomelo_pool_acquire(transporter->checksum_pool);
    if (!command) {
        // Failed to acquire new command
        pomelo_delivery_bus_update_parcel_checksum_done(
            bus, parcel, mode, NULL
        );
        return;
    }

    pomelo_delivery_fragment_t * fragment;
    ret = pomelo_unrolled_list_get_back(parcel->fragments, &fragment);
    if (ret != 0) {
        command->result = -1;
        pomelo_delivery_bus_update_parcel_checksum_done(
            bus, parcel, mode, command
        );
        return;
    }
    size_t last_fragment_capacity = fragment->payload.capacity;
    fragment->payload.capacity = fragment->payload.position;

    // Update the command information
    command->callback_mode = POMELO_DELIVERY_CHECKSUM_CALLBACK_UPDATE;
    command->bus = bus;
    command->parcel = parcel;
    command->result = 0;

    // Store specific fields for updating
    pomelo_delivery_checksum_updating_data_t * updating =
        &command->specific.updating;
    updating->delivery_mode = mode;
    updating->last_fragment = fragment;
    updating->last_fragment_capacity = last_fragment_capacity;

    ret = pomelo_platform_submit_worker_task(
        transporter->platform,
        transporter->task_group,
        (pomelo_platform_task_cb) pomelo_delivery_checksum_command_entry,
        (pomelo_platform_task_done_cb) pomelo_delivery_checksum_command_done,
        command
    );
    if (ret < 0) {
        // Failed to append work
        command->result = -1;
        pomelo_delivery_bus_update_parcel_checksum_done(
            bus, parcel, mode, command
        );
    }

    // => pomelo_delivery_bus_update_parcel_checksum_done
}


void pomelo_delivery_bus_update_parcel_checksum_done(
    pomelo_delivery_bus_t * bus,
    pomelo_delivery_parcel_t * parcel,
    pomelo_delivery_mode mode,
    pomelo_delivery_checksum_command_t * command
) {
    assert(bus != NULL);
    assert(parcel != NULL);

    if (!command) {
        // Failed to acquire new command
        pomelo_delivery_bus_preprocess_send_parcel_done(bus, parcel, mode, -1);
        return;
    }

    // Restore the capacity of last fragment to write the checksum
    pomelo_delivery_checksum_updating_data_t * updating =
        &command->specific.updating;
    updating->last_fragment->payload.capacity =
        updating->last_fragment_capacity;
    
    // Get the writer and append the checksum to the last of parcel
    pomelo_delivery_parcel_writer_t * writer =
        pomelo_delivery_parcel_get_writer(parcel);
    if (!writer) {
        // Failed to get writer
        pomelo_delivery_bus_preprocess_send_parcel_done(bus, parcel, mode, -1);
        return;
    }

    int ret = pomelo_delivery_parcel_writer_write_buffer(
        writer,
        POMELO_CODEC_CHECKSUM_BYTES,
        command->checksum
    );

    // Release the command
    pomelo_pool_release(bus->endpoint->transporter->checksum_pool, command);

    // Finally, call the callback with result
    pomelo_delivery_bus_preprocess_send_parcel_done(bus, parcel, mode, ret);
}


void pomelo_delivery_bus_preprocess_send_parcel_done(
    pomelo_delivery_bus_t * bus,
    pomelo_delivery_parcel_t * parcel,
    pomelo_delivery_mode mode,
    int result
) {
    assert(bus != NULL);
    assert(parcel != NULL);

    if (result < 0) {
        // Failed to compute checksum, unref the parcel
        pomelo_delivery_parcel_unref(parcel);
        return;
    }

    // Pack the last fragment capacity.
    pomelo_delivery_fragment_t * fragment;
    int ret;

    ret = pomelo_unrolled_list_get_back(parcel->fragments, &fragment);
    if (ret != 0) {
        // Failed to get the last fragment
        pomelo_delivery_parcel_unref(parcel);
        return;
    }
    fragment->payload.capacity = fragment->payload.position;

    // Prepare the sending command

    uint64_t sequence = bus->parcel_sequence + 1;
    pomelo_delivery_send_command_t * command =
        pomelo_delivery_send_command_prepare(bus, parcel, mode, sequence);
    if (!command) {
        // Cannot prepare a new command
        pomelo_delivery_parcel_unref(parcel);
        return;
    }

    // Update the parcel meta data
    ret = pomelo_delivery_bus_update_parcel_meta(bus, command);
    if (ret < 0) {
        pomelo_delivery_send_command_cleanup(command);
        return;
    }

    // Pack the parcel after encoding the meta
    pomelo_delivery_parcel_pack(parcel);

    /* End prepare the sending command */

    // Push the command at the end of queue
    if (!pomelo_list_push_back(bus->pending_send_commands, command)) {
        // Failed to append command
        pomelo_delivery_send_command_cleanup(command);
        return;
    }
    
    // Increase the parcel sequence
    ++bus->parcel_sequence;
    
    if (bus->incomplete_reliable_send_command) {
        // This bus is blocking
        return;
    }

    // This bus is not blocking, process the commands
    pomelo_delivery_bus_process_sending(bus);
}


int pomelo_delivery_bus_reply_ack(
    pomelo_delivery_bus_t * bus,
    pomelo_delivery_fragment_meta_t * meta
) {
    assert(bus != NULL);
    assert(meta != NULL);

    int ret;
    pomelo_delivery_endpoint_t * endpoint = bus->endpoint;
    pomelo_delivery_context_t * context = endpoint->transporter->context;
    pomelo_buffer_context_t * buffer_context = context->buffer_context;

    // Acquire new buffer for writing
    pomelo_buffer_t * buffer = buffer_context->acquire(buffer_context);
    if (!buffer) {
        return -1;
    }

    pomelo_payload_t payload;
    payload.position = 0;
    payload.capacity = context->fragment_payload_capacity;
    payload.data = buffer->data;

    pomelo_delivery_fragment_meta_t ack_meta = *meta;
    ack_meta.type = POMELO_FRAGMENT_TYPE_ACK;

    ret = pomelo_delivery_fragment_meta_encode(&ack_meta, &payload);
    if (ret < 0) {
        // Failed to encode meta
        pomelo_buffer_unref(buffer);
        return ret;
    }

    // Update the position & capacity for sending
    pomelo_payload_pack(&payload);

    // Send the payload
    ret = pomelo_delivery_endpoint_send(endpoint, buffer, 0, payload.capacity);

    // Finally, unref the payload
    pomelo_buffer_unref(buffer);
    return ret;
}
