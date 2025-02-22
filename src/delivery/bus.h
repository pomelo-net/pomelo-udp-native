#ifndef POMELO_DELIVERY_BUS_SRC_H
#define POMELO_DELIVERY_BUS_SRC_H
#include "utils/map.h"
#include "utils/list.h"
#include "fragment.h"
#include "checksum.h"

#ifdef __cplusplus
extern "C" {
#endif


struct pomelo_delivery_bus_s {
    /// @brief The endpoint which this bus belongs to
    pomelo_delivery_endpoint_t * endpoint;

    /// @brief The bus index
    size_t index;

    /// @brief The parcels which are pending because of bus blocking.
    pomelo_list_t * pending_send_commands;

    /// @brief The incomplete receiving parcels.
    pomelo_map_t * incomplete_recv_commands;

    /// @brief The reliable parcel which is receiving from other peer.
    /// No more reliable parcels will be accepted if there's a pending reliable
    /// receiving command.
    pomelo_delivery_recv_command_t * incomplete_reliable_recv_command;

    /// @brief The reliable parcel which is waiting to be acked. If this field
    /// is not null, the next incoming parcels will be queued in pending
    /// parcels list
    pomelo_delivery_send_command_t * incomplete_reliable_send_command;

    /// @brief The most recent received reliable sequence number
    uint64_t most_recent_recv_reliable_sequence;

    /// @brief The parcel sequence generator. It starts from 1.
    uint64_t parcel_sequence;

    /// @brief The newest received sequence number.
    uint64_t most_recent_recv_sequenced_sequence;
};


/// Initialize the bus
int pomelo_delivery_bus_init(
    pomelo_delivery_bus_t * bus,
    pomelo_delivery_endpoint_t * endpoint
);


/// @brief Reset the bus.
/// Cleanup all pending parcels
void pomelo_delivery_bus_reset(pomelo_delivery_bus_t * bus);


/// @brief Finalize the bus
void pomelo_delivery_bus_finalize(pomelo_delivery_bus_t * bus);


/// @brief Process when a bus receives a payload
int pomelo_delivery_bus_on_received_payload(
    pomelo_delivery_bus_t * bus,
    pomelo_delivery_fragment_meta_t * meta,
    pomelo_buffer_t * buffer,
    pomelo_payload_t * payload
);


/// @brief Process receiving fragment data
int pomelo_delivery_bus_on_received_fragment_data(
    pomelo_delivery_bus_t * bus,
    pomelo_delivery_fragment_meta_t * meta,
    pomelo_buffer_t * buffer,
    pomelo_payload_t * payload
);


/// @brief Process receiving fragment ACK
int pomelo_delivery_bus_on_received_fragment_ack(
    pomelo_delivery_bus_t * bus,
    pomelo_delivery_fragment_meta_t * meta
);


/// @brief The callback when a sending command has completed running
void pomelo_delivery_bus_on_send_command_completed(
    pomelo_delivery_bus_t * bus,
    pomelo_delivery_send_command_t * command
);


/// @brief The callback when a receiving command has completed running
void pomelo_delivery_bus_on_recv_command_completed(
    pomelo_delivery_bus_t * bus,
    pomelo_delivery_recv_command_t * command
);

/// @brief Post-process the received parcel
void pomelo_delivery_bus_postprocess_recv_parcel(
    pomelo_delivery_bus_t * bus,
    pomelo_delivery_recv_command_t * command
);

/// @brief Validate the parcel checksum
void pomelo_delivery_bus_validate_parcel_checksum(
    pomelo_delivery_bus_t * bus,
    pomelo_delivery_recv_command_t * command
);

/// @brief Callback after validate the parcel checksum
void pomelo_delivery_bus_validate_parcel_checksum_done(
    pomelo_delivery_bus_t * bus,
    pomelo_delivery_recv_command_t * command,
    pomelo_delivery_checksum_command_t * checksum_command
);

/// @brief Callback after post-processing received parcel
void pomelo_delivery_bus_postprocess_recv_parcel_done(
    pomelo_delivery_bus_t * bus,
    pomelo_delivery_recv_command_t * command,
    int result
);


/// @brief Continue sending parcels. This is the "main loop" of bus
void pomelo_delivery_bus_process_sending(
    pomelo_delivery_bus_t * bus
);


/// @brief Update the parcel meta data
int pomelo_delivery_bus_update_parcel_meta(
    pomelo_delivery_bus_t * bus,
    pomelo_delivery_send_command_t * command
);


/// @brief Process when receiving command has expired
void pomelo_delivery_bus_on_recv_command_expired(
    pomelo_delivery_bus_t * bus,
    pomelo_delivery_recv_command_t * command
);


/// @brief Process when an error has occurred when process a sending command
void pomelo_delivery_bus_on_send_command_error(
    pomelo_delivery_bus_t * bus,
    pomelo_delivery_send_command_t * command
);


/// @brief Remove the receiving command from this bus
void pomelo_delivery_bus_remove_recv_command(
    pomelo_delivery_bus_t * bus,
    pomelo_delivery_recv_command_t * command
);


/// @brief Preprocess before sending parcel
void pomelo_delivery_bus_preprocess_send_parcel(
    pomelo_delivery_bus_t * bus,
    pomelo_delivery_parcel_t * parcel,
    pomelo_delivery_mode mode
);


/// @brief Update the parcel checksum before sending
void pomelo_delivery_bus_update_parcel_checksum(
    pomelo_delivery_bus_t * bus,
    pomelo_delivery_parcel_t * parcel,
    pomelo_delivery_mode mode
);


/// @brief Process when computing checksum is completed
void pomelo_delivery_bus_update_parcel_checksum_done(
    pomelo_delivery_bus_t * bus,
    pomelo_delivery_parcel_t * parcel,
    pomelo_delivery_mode mode,
    pomelo_delivery_checksum_command_t * command
);


/// @brief The callback after updating parcel checksum
void pomelo_delivery_bus_preprocess_send_parcel_done(
    pomelo_delivery_bus_t * bus,
    pomelo_delivery_parcel_t * parcel,
    pomelo_delivery_mode mode,
    int result
);


/// @brief Reply ack
int pomelo_delivery_bus_reply_ack(
    pomelo_delivery_bus_t * bus,
    pomelo_delivery_fragment_meta_t * meta
);


#ifdef __cplusplus
}
#endif
#endif // POMELO_DELIVERY_BUS_SRC_H
