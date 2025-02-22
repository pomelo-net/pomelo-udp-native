#include <assert.h>
#include "adapter/adapter.h"
#include "socket.h"


void pomelo_adapter_on_sent(
    pomelo_adapter_t * adapter,
    void * send_data,
    int status
) {
    assert(adapter != NULL);
    pomelo_protocol_socket_t * socket = pomelo_adapter_get_extra(adapter);
    pomelo_protocol_socket_on_sent(socket, send_data, status);
}


void pomelo_adapter_on_alloc(
    pomelo_adapter_t * adapter,
    pomelo_buffer_vector_t * buffer_vector
) {
    assert(adapter != NULL);

    pomelo_protocol_socket_t * socket = pomelo_adapter_get_extra(adapter);
    pomelo_protocol_socket_on_alloc(socket, buffer_vector);
}


void pomelo_adapter_on_recv(
    pomelo_adapter_t * adapter,
    pomelo_address_t * address,
    pomelo_buffer_vector_t * buffer_vector,
    int status,
    bool encrypted
) {
    assert(adapter != NULL);
    pomelo_protocol_socket_t * socket = pomelo_adapter_get_extra(adapter);
    pomelo_protocol_socket_on_recv(
        socket,
        address,
        buffer_vector,
        status,
        encrypted
    );
}
