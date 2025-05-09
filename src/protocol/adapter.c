#include <assert.h>
#include "adapter.h"
#include "socket.h"
#include "sender.h"
#include "context.h"



pomelo_protocol_acceptance_t * pomelo_protocol_acceptance_init(
    pomelo_protocol_socket_t * socket,
    pomelo_address_t * address,
    pomelo_buffer_view_t * view,
    bool encrypted
) {
    assert(socket != NULL);
    assert(address != NULL);
    assert(view != NULL);

    // Socket is busy, pending the incoming data
    pomelo_protocol_acceptance_t * acceptance =
        pomelo_pool_acquire(socket->context->acceptance_pool, NULL);
    if (!acceptance) return NULL; // Failed to acquire acceptance

    acceptance->socket = socket;
    acceptance->address = *address;
    acceptance->view = *view;
    acceptance->encrypted = encrypted;
    acceptance->context = socket->context;
    pomelo_buffer_ref(view->buffer);

    // Initialize the task
    pomelo_sequencer_task_init(
        &acceptance->task,
        (pomelo_sequencer_callback) pomelo_protocol_acceptance_process,
        acceptance
    );

    return acceptance;
}


void pomelo_protocol_acceptance_process(
    pomelo_protocol_acceptance_t * acceptance
) {
    assert(acceptance != NULL);
    pomelo_protocol_socket_accept(
        acceptance->socket,
        &acceptance->address,
        &acceptance->view,
        acceptance->encrypted
    );

    pomelo_buffer_unref(acceptance->view.buffer);
    pomelo_pool_release(acceptance->context->acceptance_pool, acceptance);
}


void pomelo_adapter_on_recv(
    pomelo_adapter_t * adapter,
    pomelo_address_t * address,
    pomelo_buffer_view_t * view,
    bool encrypted
) {
    assert(adapter != NULL);
    pomelo_protocol_socket_t * socket = pomelo_adapter_get_extra(adapter);
    pomelo_sequencer_t * sequencer = socket->sequencer;
    
    pomelo_protocol_acceptance_t * acceptance =
        pomelo_protocol_acceptance_init(socket, address, view, encrypted);
    if (!acceptance) return; // Failed to initialize the acceptance

    // Submit the task to the sequence manager
    pomelo_sequencer_submit(sequencer, &acceptance->task);
}


pomelo_buffer_t * pomelo_adapter_buffer_acquire(pomelo_adapter_t * adapter) {
    assert(adapter != NULL);
    pomelo_protocol_socket_t * socket = pomelo_adapter_get_extra(adapter);
    return pomelo_buffer_context_acquire(socket->context->buffer_context);
}
