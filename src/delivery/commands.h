#ifndef POMELO_DELIVERY_COMMANDS_SRC_H
#define POMELO_DELIVERY_COMMANDS_SRC_H
#include "platform/platform.h"
#include "utils/list.h"
#include "transporter.h"
#include "fragment.h"

#ifdef __cplusplus
extern "C" {
#endif


struct pomelo_delivery_send_command_s {
    /// @brief The sequence number of parcel
    uint64_t sequence;

    /// @brief The sending bus
    pomelo_delivery_bus_t * bus;

    /// @brief The sending parcel
    pomelo_delivery_parcel_t * parcel;

    /// @brief The number of acknowledged fragments
    size_t acked_counter;

    /// @brief The delivery mode of this parcel
    pomelo_delivery_mode mode;

    /// @brief The timer for resending command (reliable only)
    pomelo_platform_timer_t * resend_timer;
};


struct pomelo_delivery_recv_command_s {
    /// @brief The delivery mode of this parcel
    pomelo_delivery_mode mode;

    /// @brief The sequence number of parcel
    uint64_t sequence;

    /// @brief The receiving bus
    pomelo_delivery_bus_t * bus;

    /// @brief The receiving parcel
    pomelo_delivery_parcel_t * parcel;

    /// @brief The number of received fragments
    size_t received_counter;

    /// @brief The timer for cleaning up this parcel (unreliable only)
    pomelo_platform_timer_t * expired_timer;
};



/// @brief Prepare new receiving command
pomelo_delivery_recv_command_t * pomelo_delivery_recv_command_prepare(
    pomelo_delivery_bus_t * bus,
    pomelo_delivery_fragment_meta_t * meta
);


/// @brief Prepare sending command
pomelo_delivery_send_command_t * pomelo_delivery_send_command_prepare(
    pomelo_delivery_bus_t * bus,
    pomelo_delivery_parcel_t * parcel,
    pomelo_delivery_mode mode,
    uint64_t parcel_sequence
);


/// @brief Cleanup the receiving command and its associated data
/// This function will release all the parcel data, fragments and command
/// command itself to the context.
void pomelo_delivery_recv_command_cleanup(
    pomelo_delivery_recv_command_t * command
);


/// @brief Cleanup the sending command and its associated data.
/// This function will release all the parcel data, fragments and command
/// command itself to the context.
void pomelo_delivery_send_command_cleanup(
    pomelo_delivery_send_command_t * command
);


/// @brief Start executing the sending command
void pomelo_delivery_send_command_run(pomelo_delivery_send_command_t * command);


/// @brief Start executing the receiving command
void pomelo_delivery_recv_command_run(pomelo_delivery_recv_command_t * command);


/// @brief Submit ack response to sending command
void pomelo_delivery_send_command_receive_ack(
    pomelo_delivery_send_command_t * command,
    pomelo_delivery_fragment_meta_t * meta
);


/// @brief Process when receiving command receives a fragment
/// If the fragment has been recorded, -1 will be returned.
int pomelo_delivery_recv_command_receive_fragment(
    pomelo_delivery_recv_command_t * command,
    pomelo_delivery_fragment_t * fragment
);

/// @brief Process when the receiving command has expired
void pomelo_delivery_recv_command_on_expired(
    pomelo_delivery_recv_command_t * command
);

/// @brief Deliver the un-acked fragments of sending command
void pomelo_delivery_send_command_deliver(
    pomelo_delivery_send_command_t * command
);


#ifdef __cplusplus
}
#endif
#endif // POMELO_DELIVERY_COMMANDS_SRC_H
