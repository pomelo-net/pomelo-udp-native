#ifndef POMELO_DELIVERY_SRC_H
#define POMELO_DELIVERY_SRC_H
#include "pomelo/allocator.h"
#include "pomelo/statistic/statistic-delivery.h"
#include "base/buffer.h"
#include "base/payload.h"
#include "base/sequencer.h"
#include "platform/platform.h"
#ifdef __cplusplus
extern "C" {
#endif


/// The maximum bytes of meta data of fragment
#define POMELO_MAX_FRAGMENT_META_DATA_BYTES 15

/// The maximum number of fragments this protocol can support
#define POMELO_DELIVERY_PARCEL_MAX_FRAGMENTS 65536

/// The maximum number of fragments in a parcel.
/// By the default specs, each fragment can store (1200 - 15) bytes of data.
/// So that, 222 fragments can store upto 263070 bytes ~ 256KB.
#define POMELO_DELIVERY_PARCEL_MAX_FRAGMENTS_DEFAULT 222

/// The maximum number of buses
#define POMELO_DELIVERY_MAX_BUSES 65535


/// @brief Delivery mode
typedef enum pomelo_delivery_mode {
    /// @brief The unreliable mode
    POMELO_DELIVERY_MODE_UNRELIABLE,

    /// @brief The sequenced mode
    POMELO_DELIVERY_MODE_SEQUENCED,

    /// @brief The reliable mode
    POMELO_DELIVERY_MODE_RELIABLE

} pomelo_delivery_mode;


/// @brief The delivery context interface.
typedef struct pomelo_delivery_context_s pomelo_delivery_context_t;


/// @brief The non-threadsafe delivery context
typedef struct pomelo_delivery_context_shared_options_s
    pomelo_delivery_context_shared_options_t;


/// @brief The delivery endpoint.
/// This contains a group of buses and the some statistic values about
/// status of the connection.
typedef struct pomelo_delivery_endpoint_s pomelo_delivery_endpoint_t;

/// @brief The delivery bus
typedef struct pomelo_delivery_bus_s pomelo_delivery_bus_t;

/// @brief The delivery context creating options
typedef struct pomelo_delivery_context_root_options_s
    pomelo_delivery_context_root_options_t;

/// @brief The data for transmitting. It contains multiple fragments.
typedef struct pomelo_delivery_parcel_s pomelo_delivery_parcel_t;

/// @brief The fragment of a parcel
typedef struct pomelo_delivery_fragment_s pomelo_delivery_fragment_t;

/// @brief The parcel reader.
typedef struct pomelo_delivery_reader_s pomelo_delivery_reader_t;

/// @brief The parcel writer.
typedef struct pomelo_delivery_writer_s pomelo_delivery_writer_t;

/// @brief The sender of transport
typedef struct pomelo_delivery_sender_s pomelo_delivery_sender_t;

/// @brief The heartbeat of delivery
typedef struct pomelo_delivery_heartbeat_s pomelo_delivery_heartbeat_t;

/// @brief The options of the endpoint
typedef struct pomelo_delivery_endpoint_options_s
    pomelo_delivery_endpoint_options_t;

/// @brief The options of sender
typedef struct pomelo_delivery_sender_options_s
    pomelo_delivery_sender_options_t;

/// @brief The options of heartbeat
typedef struct pomelo_delivery_heartbeat_options_s
    pomelo_delivery_heartbeat_options_t;


struct pomelo_delivery_context_root_options_s {
    /// @brief The allocator of delivery context
    pomelo_allocator_t * allocator;

    /// @brief The global buffer context
    pomelo_buffer_context_t * buffer_context;

    /// @brief The total capacity of a packet
    size_t fragment_capacity;

    /// @brief The maximum number of fragments in a parcel
    size_t max_fragments;

    /// @brief Whether to synchronize the context
    bool synchronized;
};


struct pomelo_delivery_context_shared_options_s {
    /// @brief The allocator
    pomelo_allocator_t * allocator;

    /// @brief The referenced context. This will be used to get the root
    /// context. The root context must be synchronized.
    pomelo_delivery_context_t * origin_context;
};


struct pomelo_delivery_endpoint_options_s {
    /// @brief The context of this endpoint
    pomelo_delivery_context_t * context;

    /// @brief The platform
    pomelo_platform_t * platform;

    /// @brief The sequencer
    pomelo_sequencer_t * sequencer;

    /// @brief The heartbeat used by this endpoint
    pomelo_delivery_heartbeat_t * heartbeat;

    /// @brief The number of buses of this endpoint
    size_t nbuses;

    /// @brief Whether to sync time. This is for client side.
    bool time_sync;
};


struct pomelo_delivery_sender_options_s {
    /// @brief The context of this command
    pomelo_delivery_context_t * context;

    /// @brief The platform of this command
    pomelo_platform_t * platform;

    /// @brief The parcel of this command
    pomelo_delivery_parcel_t * parcel;
};


struct pomelo_delivery_heartbeat_options_s {
    /// @brief Delivery context
    pomelo_delivery_context_t * context;

    /// @brief Platform
    pomelo_platform_t * platform;
};


struct pomelo_delivery_reader_s {
    /// @brief The parcel
    pomelo_delivery_parcel_t * parcel;

    /// @brief The current reading payload
    pomelo_payload_t payload;

    /// @brief The current index of chunk
    size_t index;

    /// @brief The number of remain bytes
    size_t remain_bytes;
};


struct pomelo_delivery_writer_s {
    /// @brief The parcel
    pomelo_delivery_parcel_t * parcel;

    /// @brief The number of written bytes of this parcel
    size_t written_bytes;
};


/* -------------------------------------------------------------------------- */
/*                           Delivery context APIs                            */
/* -------------------------------------------------------------------------- */

/// @brief Create new root context
pomelo_delivery_context_t * pomelo_delivery_context_root_create(
    pomelo_delivery_context_root_options_t * options
);


/// @brief Create new shared context
pomelo_delivery_context_t * pomelo_delivery_context_shared_create(
    pomelo_delivery_context_shared_options_t * options
);


/// @brief Destroy the delivery context
void pomelo_delivery_context_destroy(pomelo_delivery_context_t * context);


/// @brief Get statistic of context
void pomelo_delivery_context_statistic(
    pomelo_delivery_context_t * context,
    pomelo_statistic_delivery_t * statistic
);


/// @brief Acquire a parcel from context
pomelo_delivery_parcel_t * pomelo_delivery_context_acquire_parcel(
    pomelo_delivery_context_t * context
);


/* -------------------------------------------------------------------------- */
/*                               Heartbeat APIs                               */
/* -------------------------------------------------------------------------- */

/// @brief Create new heartbeat
pomelo_delivery_heartbeat_t * pomelo_delivery_heartbeat_create(
    pomelo_delivery_heartbeat_options_t * options
);


/// @brief Destroy a heartbeat
void pomelo_delivery_heartbeat_destroy(pomelo_delivery_heartbeat_t * heartbeat);


/* -------------------------------------------------------------------------- */
/*                                Sender APIs                                 */
/* -------------------------------------------------------------------------- */


/// @brief Create new sender
pomelo_delivery_sender_t * pomelo_delivery_sender_create(
    pomelo_delivery_sender_options_t * options
);


/// @brief Set sender extra data
void pomelo_delivery_sender_set_extra(
    pomelo_delivery_sender_t * sender,
    void * data
);


/// @brief Get sender extra data
void * pomelo_delivery_sender_get_extra(pomelo_delivery_sender_t * sender);


/// @brief Add a transmission to the sender
int pomelo_delivery_sender_add_transmission(
    pomelo_delivery_sender_t * sender,
    pomelo_delivery_bus_t * bus,
    pomelo_delivery_mode mode
);


/// @brief Submit a sender
void pomelo_delivery_sender_submit(pomelo_delivery_sender_t * sender);


/// @brief Cancel a sender
void pomelo_delivery_sender_cancel(pomelo_delivery_sender_t * sender);


/// @brief Process when sender is completed
/// [External linkage]
void pomelo_delivery_sender_on_result(
    pomelo_delivery_sender_t * sender,
    pomelo_delivery_parcel_t * parcel,
    size_t transmission_count
);


/* -------------------------------------------------------------------------- */
/*                               Endpoint APIs                                */
/* -------------------------------------------------------------------------- */

pomelo_delivery_endpoint_t * pomelo_delivery_endpoint_create(
    pomelo_delivery_endpoint_options_t * options
);


/// @brief Destroy an endpoint
void pomelo_delivery_endpoint_destroy(pomelo_delivery_endpoint_t * endpoint);


/// @brief Start the endpoint
int pomelo_delivery_endpoint_start(pomelo_delivery_endpoint_t * endpoint);


/// @brief Stop the endpoint
void pomelo_delivery_endpoint_stop(pomelo_delivery_endpoint_t * endpoint);


/// @brief Get the endpoint extra data
void * pomelo_delivery_endpoint_get_extra(
    pomelo_delivery_endpoint_t * endpoint
);


/// @brief Set the extra data for endpoint
void pomelo_delivery_endpoint_set_extra(
    pomelo_delivery_endpoint_t * endpoint,
    void * data
);


/// @brief Get the bus from endpoint
pomelo_delivery_bus_t * pomelo_delivery_endpoint_get_bus(
    pomelo_delivery_endpoint_t * endpoint,
    size_t index
);


/// @brief Process when enpoint has received data
int pomelo_delivery_endpoint_recv(
    pomelo_delivery_endpoint_t * endpoint,
    pomelo_buffer_view_t * view
);


/// @brief Get round trip time information of endpoint
void pomelo_delivery_endpoint_rtt(
    pomelo_delivery_endpoint_t * endpoint,
    uint64_t * mean,
    uint64_t * variance
);


/// @brief Get the current time offset of endpoint
int64_t pomelo_delivery_endpoint_time_offset(
    pomelo_delivery_endpoint_t * endpoint
);


/// @brief Process sending the fragment
/// [External linkage]
int pomelo_delivery_endpoint_send(
    pomelo_delivery_endpoint_t * endpoint,
    pomelo_buffer_view_t * views,
    size_t nviews
);


/// @brief Process when endpoint is ready
/// [External linkage]
void pomelo_delivery_endpoint_on_ready(pomelo_delivery_endpoint_t * endpoint);


/* -------------------------------------------------------------------------- */
/*                                 Bus APIs                                   */
/* -------------------------------------------------------------------------- */

/// @brief Set the extra data for bus
void pomelo_delivery_bus_set_extra(
    pomelo_delivery_bus_t * bus,
    void * extra
);


/// @brief Get the extra data of bus
void * pomelo_delivery_bus_get_extra(pomelo_delivery_bus_t * bus);


/// @brief Get the endpoint from bus
pomelo_delivery_endpoint_t * pomelo_delivery_bus_get_endpoint(
    pomelo_delivery_bus_t * bus
);


/// @brief Process when bus receives a parcel.
/// After this callback is called, the parcel will immediately be released.
/// [External linkage]
void pomelo_delivery_bus_on_received(
    pomelo_delivery_bus_t * bus,
    pomelo_delivery_parcel_t * parcel,
    pomelo_delivery_mode mode
);


/* -------------------------------------------------------------------------- */
/*                                Parcel APIs                                 */
/* -------------------------------------------------------------------------- */

/// @brief Set the extra data for parcel
void pomelo_delivery_parcel_set_extra(
    pomelo_delivery_parcel_t * parcel,
    void * extra
);


/// @brief Get the extra data of parcel
void * pomelo_delivery_parcel_get_extra(pomelo_delivery_parcel_t * parcel);


/// @brief Increase the reference counter of this parcel.
bool pomelo_delivery_parcel_ref(pomelo_delivery_parcel_t * parcel);


/// @brief Decrease the reference counter of this parcel
void pomelo_delivery_parcel_unref(pomelo_delivery_parcel_t * parcel);


/// @brief Reset the parcel
void pomelo_delivery_parcel_reset(pomelo_delivery_parcel_t * parcel);


/// @brief Initialize the reader of parcel
void pomelo_delivery_reader_init(
    pomelo_delivery_reader_t * reader,
    pomelo_delivery_parcel_t * parcel
);


/// @brief Initialize the writer of parcel
void pomelo_delivery_writer_init(
    pomelo_delivery_writer_t * writer,
    pomelo_delivery_parcel_t * parcel
);


/// @brief Write buffer to the parcel
int pomelo_delivery_writer_write(
    pomelo_delivery_writer_t * writer,
    const uint8_t * buffer,
    size_t length
);


/// @brief Get the written bytes of writing parcel
/// @return The number of written bytes
size_t pomelo_delivery_writer_written_bytes(pomelo_delivery_writer_t * writer);


/// @brief Reader buffer from parcel
int pomelo_delivery_reader_read(
    pomelo_delivery_reader_t * reader,
    uint8_t * buffer,
    size_t length
);


/// @brief Get the remain available bytes of reading parcel
/// @param reader The reader of parcel
/// @return The remain bytes of parcel
size_t pomelo_delivery_reader_remain_bytes(pomelo_delivery_reader_t * reader);


#ifdef __cplusplus
}
#endif
#endif // POMELO_DELIVERY_SRC_H
