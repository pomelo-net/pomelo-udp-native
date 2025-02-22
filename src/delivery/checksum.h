#ifndef POMELO_DELIVERY_CHECKSUM_SRC_H
#define POMELO_DELIVERY_CHECKSUM_SRC_H
#include "delivery.h"
#include "codec/checksum.h"
#include "fragment.h"
#include "commands.h"


#ifdef __cplusplus
extern "C" {
#endif


/// @brief Checksum callback mode
typedef enum pomelo_delivery_checksum_callback_mode {
    /// @brief Update callback mode
    POMELO_DELIVERY_CHECKSUM_CALLBACK_UPDATE,

    /// @brief Validate callback mode
    POMELO_DELIVERY_CHECKSUM_CALLBACK_VALIDATE
} pomelo_delivery_checksum_callback_mode;


/// @brief Checksum computing command
typedef struct pomelo_delivery_checksum_command_s
    pomelo_delivery_checksum_command_t;


typedef struct pomelo_delivery_checksum_updating_data_s {
    /// @brief The delivery mode
    pomelo_delivery_mode delivery_mode;

    /// @brief The last fragment of parcel
    pomelo_delivery_fragment_t * last_fragment;

    /// @brief The temporary value for update parcel checksum process
    size_t last_fragment_capacity;
} pomelo_delivery_checksum_updating_data_t;


typedef struct pomelo_delivery_checksum_validating_data_s {
    /// @brief The receiving command
    pomelo_delivery_recv_command_t * recv_command;

    /// @brief The embedded checksum in the content of parcel
    uint8_t embedded_checksum[POMELO_CODEC_CHECKSUM_BYTES];
} pomelo_delivery_checksum_validating_data_t;


typedef union pomelo_delivery_checksum_specific {
    /// @brief The updating specific data
    pomelo_delivery_checksum_updating_data_t updating;

    /// @brief The validating specific data
    pomelo_delivery_checksum_validating_data_t validating;
} pomelo_delivery_checksum_specific_t;


struct pomelo_delivery_checksum_command_s {
    /// @brief Computing result
    int result;

    /// @brief The checksum output
    uint8_t checksum[POMELO_CODEC_CHECKSUM_BYTES];

    /// @brief Callback mode
    pomelo_delivery_checksum_callback_mode callback_mode;

    /// @brief The requesting bus
    pomelo_delivery_bus_t * bus;

    /// @brief The input parcel
    pomelo_delivery_parcel_t * parcel;

    /// @brief The specific data for each mode
    pomelo_delivery_checksum_specific_t specific;
};


/// @brief Compute the checksum
int pomelo_delivery_checksum_compute(
    uint8_t * checksum,
    pomelo_delivery_parcel_t * parcel
);


/// @brief Entry point of checksum computing
void pomelo_delivery_checksum_command_entry(
    pomelo_delivery_checksum_command_t * command
);


/// @brief Done point of checksum computing
void pomelo_delivery_checksum_command_done(
    pomelo_delivery_checksum_command_t * command,
    bool canceled  
);


#ifdef __cplusplus
}
#endif
#endif // POMELO_DELIVERY_CHECKSUM_SRC_H
