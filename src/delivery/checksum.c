#include <assert.h>
#include "checksum.h"
#include "parcel.h"
#include "bus.h"


int pomelo_delivery_checksum_compute(
    uint8_t * checksum,
    pomelo_delivery_parcel_t * parcel
) {
    assert(checksum != NULL);
    assert(parcel != NULL);

    pomelo_delivery_fragment_t * fragment;
    pomelo_codec_checksum_state_t state;
    pomelo_unrolled_list_iterator_t it;
    pomelo_payload_t payload;

    int ret = pomelo_codec_checksum_init(&state);
    if (ret < 0) return ret;

    pomelo_unrolled_list_begin(parcel->fragments, &it);
    while (pomelo_unrolled_list_iterator_next(&it, &fragment)) {
        // Capture the payload from begin position (allocated point)
        payload = fragment->payload;
        payload.position = 0;

        ret = pomelo_codec_checksum_update(&state, &payload);
        if (ret < 0) return ret;
    }

    return pomelo_codec_checksum_final(&state, checksum);
}


void pomelo_delivery_checksum_command_entry(
    pomelo_delivery_checksum_command_t * command
) {
    assert(command != NULL);
    command->result = 
        pomelo_delivery_checksum_compute(command->checksum, command->parcel);
}


void pomelo_delivery_checksum_command_done(
    pomelo_delivery_checksum_command_t * command,
    bool canceled  
) {
    assert(command != NULL);

    if (canceled) {
        command->result = -1;
    }

    if (command->callback_mode == POMELO_DELIVERY_CHECKSUM_CALLBACK_UPDATE) {
        // Call the update callback
        pomelo_delivery_bus_update_parcel_checksum_done(
            command->bus,
            command->parcel,
            command->specific.updating.delivery_mode,
            command
        );
    } else {
        pomelo_delivery_bus_validate_parcel_checksum_done(
            command->bus,
            command->specific.validating.recv_command,
            command
        );
    }
}
