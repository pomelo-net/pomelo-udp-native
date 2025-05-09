#ifndef POMELO_DELIVERY_BUS_SRC_H
#define POMELO_DELIVERY_BUS_SRC_H
#include "utils/map.h"
#include "utils/list.h"
#include "utils/heap.h"
#include "base/extra.h"
#include "internal.h"
#include "fragment.h"
#ifdef __cplusplus
extern "C" {
#endif


/// @brief The processing flag
#define POMELO_DELIVERY_BUS_FLAG_PROCESSING        (1 << 0)

/// @brief The stopping flag
#define POMELO_DELIVERY_BUS_FLAG_STOP              (1 << 1)

/// @brief The information of receiving
typedef struct pomelo_delivery_reception_s pomelo_delivery_reception_t;

/// @brief The information of bus
typedef struct pomelo_delivery_bus_info_s pomelo_delivery_bus_info_t;


struct pomelo_delivery_reception_s {
    /// @brief The bus which this information belongs to
    pomelo_delivery_bus_t * bus;

    /// @brief The meta of the fragment
    pomelo_delivery_fragment_meta_t meta;

    /// @brief The content of the fragment
    pomelo_buffer_view_t content;

    /// @brief The receive task
    pomelo_sequencer_task_t task;
};


struct pomelo_delivery_bus_info_s {
    /// @brief The endpoint which this bus belongs to
    pomelo_delivery_endpoint_t * endpoint;

    /// @brief The id of this bus
    size_t id;
};


struct pomelo_delivery_bus_s {
    /// @brief The extra data of bus
    pomelo_extra_t extra;

    /// @brief The endpoint which this bus belongs to
    pomelo_delivery_endpoint_t * endpoint;

    /// @brief The id of this bus
    size_t id;

    /// @brief The context of this bus
    pomelo_delivery_context_t * context;

    /// @brief The platform of this bus
    pomelo_platform_t * platform;

    /// @brief The dispatchers which are pending because bus is blocked
    pomelo_list_t * pending_dispatchers;

    /// @brief The map of receivers by sequence
    pomelo_map_t * receivers_map;

    /// @brief The heap of receivers by expired time
    pomelo_heap_t * receivers_heap;

    /// @brief The reliable parcel which is receiving from other peer.
    /// No more reliable parcels will be accepted if there's a pending reliable
    /// receiving command.
    pomelo_delivery_receiver_t * incomplete_reliable_receiver;

    /// @brief The reliable parcel which is waiting to be acked. If this field
    /// is not null, the next incoming parcels will be queued in pending
    /// dispatchers list
    pomelo_delivery_dispatcher_t * incomplete_reliable_dispatcher;

    /// @brief The last received reliable parcel sequence number
    uint64_t last_recv_reliable_sequence;

    /// @brief The last received sequenced parcel sequence number
    uint64_t last_recv_sequenced_sequence;

    /// @brief The parcel sequence generator. It starts from 1.
    uint64_t sequence_generator;

    /// @brief The flags of bus
    uint32_t flags;

    /// @brief The send task
    pomelo_sequencer_task_t send_task;
};


/// @brief The callback when a bus is allocated
int pomelo_delivery_bus_on_alloc(
    pomelo_delivery_bus_t * bus,
    pomelo_delivery_context_t * context
);


/// @brief The callback when a bus is freed
void pomelo_delivery_bus_on_free(pomelo_delivery_bus_t * bus);


/// @brief Initialize the bus
int pomelo_delivery_bus_init(
    pomelo_delivery_bus_t * bus,
    pomelo_delivery_bus_info_t * info
);


/// @brief Cleanup the bus
void pomelo_delivery_bus_cleanup(pomelo_delivery_bus_t * bus);


/// @brief Start the bus
int pomelo_delivery_bus_start(pomelo_delivery_bus_t * bus);


/// @brief Stop all checksum tasks
void pomelo_delivery_bus_stop(pomelo_delivery_bus_t * bus);


/* -------------------------------------------------------------------------- */
/*                             Receiving Process                              */
/* -------------------------------------------------------------------------- */


/// @brief Process when a bus receives a payload
int pomelo_delivery_bus_recv(
    pomelo_delivery_bus_t * bus,
    pomelo_delivery_fragment_meta_t * meta,
    pomelo_buffer_view_t * view_content
);


/// @brief Process receiving fragment data
int pomelo_delivery_bus_recv_fragment_data(
    pomelo_delivery_bus_t * bus,
    pomelo_delivery_fragment_meta_t * meta,
    pomelo_buffer_view_t * view_content
);


/// @brief Handle the completion of receiving command
void pomelo_delivery_bus_handle_receiver_complete(
    pomelo_delivery_bus_t * bus,
    pomelo_delivery_receiver_t * command
);


/// @brief Process receiving fragment ACK
int pomelo_delivery_bus_recv_fragment_ack(
    pomelo_delivery_bus_t * bus,
    pomelo_delivery_fragment_meta_t * meta
);


/// @brief Reply ack
int pomelo_delivery_bus_reply_ack(
    pomelo_delivery_bus_t * bus,
    pomelo_delivery_fragment_meta_t * meta
);


/// @brief Find or create a receiving command
pomelo_delivery_receiver_t * pomelo_delivery_bus_ensure_recv_command(
    pomelo_delivery_bus_t * bus,
    pomelo_delivery_fragment_meta_t * meta
);


/// @brief Cleanup expired receiving commands
void pomelo_delivery_bus_cleanup_expired_receivers(
    pomelo_delivery_bus_t * bus
);


/// @brief Dispatch the received parcel event
void pomelo_delivery_bus_dispatch_received(
    pomelo_delivery_bus_t * bus,
    pomelo_delivery_parcel_t * parcel,
    pomelo_delivery_mode mode
);


/// @brief Initialize the reception
pomelo_delivery_reception_t * pomelo_delivery_reception_init(
    pomelo_delivery_bus_t * bus,
    pomelo_delivery_fragment_meta_t * meta,
    pomelo_buffer_view_t * view_content
);


/// @brief Execute the reception
void pomelo_delivery_reception_execute(pomelo_delivery_reception_t * reception);


/* -------------------------------------------------------------------------- */
/*                              Sending Process                               */
/* -------------------------------------------------------------------------- */

/// @brief The callback after updating parcel checksum
void pomelo_delivery_bus_dispatch_parcel(
    pomelo_delivery_bus_t * bus,
    pomelo_delivery_parcel_t * parcel
);


/// @brief Handle the parcel which has been updated
void pomelo_delivery_bus_handle_parcel_checksum_updated(
    pomelo_delivery_bus_t * bus,
    pomelo_delivery_parcel_t * parcel
);


/// @brief Continue sending parcels. This is the "main loop" of bus
void pomelo_delivery_bus_process_sending(pomelo_delivery_bus_t * bus);


/// @brief Process the sending task
void pomelo_delivery_bus_process_sending_deferred(pomelo_delivery_bus_t * bus);


/// @brief The callback when a sending dispatcher has completed running
void pomelo_delivery_bus_on_dispatcher_completed(
    pomelo_delivery_bus_t * bus,
    pomelo_delivery_dispatcher_t * dispatcher
);


#ifdef __cplusplus
}
#endif
#endif // POMELO_DELIVERY_BUS_SRC_H
