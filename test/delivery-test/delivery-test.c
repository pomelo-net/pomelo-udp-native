#include "uv.h"
#include "pomelo-test.h"
#include "base/packet.h"
#include "delivery/delivery.h"
#include "delivery/parcel.h"
#include "platform/uv/platform-uv.h"
#include "delivery/transporter.h"
#include "delivery/context.h"
#include "codec/codec.h"


/// The buffer size for transport
#define POMELO_TEST_DELIVERY_BUFFER_LENGTH 1024


// Environment
static uv_loop_t uv_loop;
static pomelo_allocator_t * allocator;
static pomelo_platform_t * platform;
static pomelo_delivery_transporter_t * transporter;

// Endpoints
static pomelo_delivery_endpoint_t * sender;
static pomelo_delivery_endpoint_t * receiver;

// Contexts
static pomelo_buffer_context_root_t * buffer_ctx;
static pomelo_delivery_context_root_t * transport_ctx;

// Temp variables


int pomelo_delivery_endpoint_rtt(
    pomelo_delivery_endpoint_t * endpoint,
    uint64_t * mean,
    uint64_t * variance
) {
    (void) endpoint;
    if (mean) {
        *mean = 0;
    }
    if (variance) {
        *variance = 0;
    }
    return 0;
}


void pomelo_delivery_transporter_on_stopped(
    pomelo_delivery_transporter_t * transporter
) {
    (void) transporter;
}


void pomelo_delivery_bus_on_received(
    pomelo_delivery_bus_t * channel,
    pomelo_delivery_parcel_t * message
) {
    pomelo_track_function();
    (void) channel;

    // Receiver will receive this message
    printf(
        "[i] Received message with %zu fragments\n",
        message->fragments->size
    );

    // Check the message data
    uint8_t byte;
    int i = 0;
    pomelo_delivery_parcel_reader_t * reader =
        pomelo_delivery_parcel_get_reader(message);
    size_t remain_bytes = pomelo_relivery_parcel_reader_remain_bytes(reader);
    printf("[i] Message size = %zu\n", remain_bytes);

    while (pomelo_delivery_parcel_reader_read_buffer(reader, 1, &byte) == 0) {
        pomelo_check(byte == i % 256);
        i++;
    }

    printf("[i] Message data is valid, bytes = %d\n", i);
    pomelo_delivery_transporter_stop(transporter);
}


int pomelo_delivery_endpoint_send(
    pomelo_delivery_endpoint_t * transporter,
    pomelo_buffer_t * buffer,
    size_t offset,
    size_t length
) {
    pomelo_track_function();
    printf("[i] Transporter sends payload with length: %zu\n", length);

    // Clone the payload
    pomelo_delivery_endpoint_t * target = (transporter == receiver)
        ? sender
        : receiver;
    int ret = pomelo_delivery_endpoint_recv(target, buffer, offset, length);
    pomelo_check(ret == 0);

    return 0;
}


/* End of implemented APIs */


int main(void) {
    printf("Delivery test\n");
    if (pomelo_codec_init() < 0) {
        printf("Failed to initialize codec \n");
        return -1;
    }

    allocator = pomelo_allocator_default();
    uint64_t alloc_bytes = pomelo_allocator_allocated_bytes(allocator);

    uv_loop_init(&uv_loop);

    // Create data context
    pomelo_buffer_context_root_options_t buffer_ctx_options;
    pomelo_buffer_context_root_options_init(&buffer_ctx_options);
    buffer_ctx_options.allocator = allocator;
    buffer_ctx_options.buffer_capacity = POMELO_PACKET_BUFFER_CAPACITY_DEFAULT;

    buffer_ctx = pomelo_buffer_context_root_create(&buffer_ctx_options);
    pomelo_check(buffer_ctx != NULL);

    // Create the platform
    pomelo_platform_uv_options_t platform_options;
    pomelo_platform_uv_options_init(&platform_options);
    platform_options.allocator = allocator;
    platform_options.uv_loop = &uv_loop;

    platform = pomelo_platform_uv_create(&platform_options);
    pomelo_check(platform != NULL);

    // Create transport context
    pomelo_delivery_context_root_options_t context_options;
    pomelo_delivery_context_root_options_init(&context_options);
    context_options.allocator = allocator;
    context_options.buffer_context = buffer_ctx;
    context_options.fragment_capacity = 
        POMELO_PACKET_BUFFER_CAPACITY_DEFAULT - POMELO_PACKET_HEADER_CAPACITY;

    transport_ctx = pomelo_delivery_context_root_create(&context_options);
    pomelo_check(transport_ctx != NULL);

    pomelo_delivery_transporter_options_t transporter_options;
    pomelo_delivery_transporter_options_init(&transporter_options);

    transporter_options.nbuses = 5;
    transporter_options.context = (pomelo_delivery_context_t *) transport_ctx;
    transporter_options.platform = platform;

    transporter = pomelo_delivery_transporter_create(&transporter_options);
    pomelo_check(transporter != NULL);

    sender = pomelo_delivery_transporter_acquire_endpoint(transporter);
    pomelo_check(sender != NULL);

    receiver = pomelo_delivery_transporter_acquire_endpoint(transporter);
    pomelo_check(receiver != NULL);

    // Get the first bus for sending
    pomelo_delivery_bus_t * bus = pomelo_delivery_endpoint_get_bus(sender, 0);
    pomelo_check(bus != NULL);

    /* Start testing */

    pomelo_delivery_parcel_t * message =
        pomelo_delivery_context_root_acquire_parcel(transport_ctx);
    pomelo_check(message != NULL);

    pomelo_delivery_parcel_writer_t * writer =
        pomelo_delivery_parcel_get_writer(message);

    int ret = 0;
    uint8_t byte;
    for (int i = 0; i < POMELO_TEST_DELIVERY_BUFFER_LENGTH; i++) {
        byte = i % 256;
        ret = pomelo_delivery_parcel_writer_write_buffer(writer, 1, &byte);
        pomelo_check(ret == 0);
    };

    // Deliver the message
    ret = pomelo_delivery_bus_send(bus, message, POMELO_DELIVERY_MODE_UNRELIABLE);
    pomelo_check(ret == 0);

    uv_run(&uv_loop, UV_RUN_DEFAULT);
    uv_loop_close(&uv_loop);

    /* End testing */

    pomelo_delivery_transporter_release_endpoint(transporter, sender);
    pomelo_delivery_transporter_release_endpoint(transporter, receiver);
    pomelo_delivery_transporter_destroy(transporter);
    pomelo_delivery_context_root_destroy(transport_ctx);
    pomelo_platform_uv_destroy(platform);
    pomelo_buffer_context_root_destroy(buffer_ctx);

    pomelo_check(alloc_bytes == pomelo_allocator_allocated_bytes(allocator));
    printf("Transport test passed\n");
    return 0;
}
