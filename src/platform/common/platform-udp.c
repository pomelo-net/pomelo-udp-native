#include <assert.h>
#include <stdbool.h>
#include "base/constants.h"
#include "platform-udp.h"


/* -------------------------------------------------------------------------- */
/*                                Public APIs                                 */
/* -------------------------------------------------------------------------- */

pomelo_platform_udp_controller_t * pomelo_platform_udp_controller_create(
    pomelo_allocator_t * allocator,
    uv_loop_t * uv_loop
) {
    assert(allocator != NULL);
    assert(uv_loop != NULL);

    pomelo_platform_udp_controller_t * controller = pomelo_allocator_malloc_t(
        allocator,
        pomelo_platform_udp_controller_t
    );

    if (!controller) {
        return NULL;
    }

    memset(controller, 0, sizeof(pomelo_platform_udp_controller_t));
    controller->allocator = allocator;
    controller->uv_loop = uv_loop;

    // Create send object pool
    pomelo_pool_options_t pool_options;
    pomelo_pool_options_init(&pool_options);
    pool_options.allocator = allocator;
    pool_options.element_size = sizeof(pomelo_platform_send_t);

    controller->send_pool = pomelo_pool_create(&pool_options);
    if (!controller->send_pool) {
        pomelo_platform_udp_controller_destroy(controller);
        return NULL;
    }

    // Create pool of sockets
    pomelo_pool_options_init(&pool_options);
    pool_options.allocator = allocator;
    pool_options.element_size = sizeof(pomelo_platform_udp_t);
    pool_options.zero_initialized = true;

    controller->socket_pool = pomelo_pool_create(&pool_options);
    if (!controller->socket_pool) {
        pomelo_platform_udp_controller_destroy(controller);
        return NULL;
    }

    // Create list of running sockets
    pomelo_list_options_t list_options;
    pomelo_list_options_init(&list_options);

    list_options.allocator = allocator;
    list_options.element_size = sizeof(pomelo_platform_udp_t *);
    controller->sockets = pomelo_list_create(&list_options);
    if (!controller->sockets) {
        pomelo_platform_udp_controller_destroy(controller);
        return NULL;
    }

    return controller;
}


void pomelo_platform_udp_controller_destroy(
    pomelo_platform_udp_controller_t * controller
) {
    assert(controller != NULL);
    pomelo_allocator_t * allocator = controller->allocator;

    if (controller->send_pool) {
        pomelo_pool_destroy(controller->send_pool);
        controller->send_pool = NULL;
    }

    if (controller->socket_pool) {
        pomelo_pool_destroy(controller->socket_pool);
        controller->socket_pool = NULL;
    }

    if (controller->sockets) {
        pomelo_list_destroy(controller->sockets);
        controller->sockets = NULL;
    }

    pomelo_allocator_free(allocator, controller);
}


void pomelo_platform_udp_controller_statistic(
    pomelo_platform_udp_controller_t * controller,
    pomelo_statistic_platform_t * statistic
) {
    assert(controller != NULL);
    assert(statistic != NULL);

    statistic->send_commands = pomelo_pool_in_use(controller->send_pool);
    statistic->sent_bytes = controller->send_bytes;
    statistic->recv_bytes = controller->recv_bytes;
}


void pomelo_platform_udp_controller_startup(
    pomelo_platform_udp_controller_t * controller
) {
    (void) controller; // Nothing to do
}


void pomelo_platform_udp_controller_shutdown(
    pomelo_platform_udp_controller_t * controller
) {
    assert(controller != NULL);

    pomelo_platform_udp_t * socket;
    pomelo_list_ptr_for(controller->sockets, socket, {
        pomelo_platform_udp_close(socket);
    });
}


pomelo_platform_udp_t * pomelo_platform_udp_controller_bind(
    pomelo_platform_udp_controller_t * controller,
    pomelo_address_t * address
) {
    assert(controller != NULL);
    assert(address != NULL);

    struct sockaddr_storage addr;
    if (pomelo_address_to_sockaddr(address, &addr) < 0) {
        return NULL;
    }

    pomelo_platform_udp_t * socket =
        pomelo_pool_acquire(controller->socket_pool);
    if (!socket) {
        return NULL;
    }
    
    // Setup socket
    socket->controller = controller;

    // Setup UDP handle
    uv_udp_t * udp = &socket->uv_udp;
    uv_udp_init(controller->uv_loop, udp);
    udp->data = socket;

    int send_buf_size = POMELO_SERVER_SOCKET_SNDBUF_SIZE;
    int recv_buf_size = POMELO_SERVER_SOCKET_RCVBUF_SIZE;

    uv_send_buffer_size((uv_handle_t *) udp, &send_buf_size);
    uv_recv_buffer_size((uv_handle_t *) udp, &recv_buf_size);

    if (uv_udp_bind(udp, (struct sockaddr *) &addr, UV_UDP_REUSEADDR) < 0) {
        pomelo_platform_udp_close(socket);
        return NULL;
    }

    int ret = uv_udp_recv_start(
        udp,
        pomelo_platform_udp_alloc_callback,
        pomelo_platform_udp_recv_callback
    );
    if (ret < 0) {
        pomelo_platform_udp_close(socket);
        return NULL;
    }

    // Push socket to list
    socket->node = pomelo_list_push_back(controller->sockets, socket);
    if (!socket->node) {
        pomelo_platform_udp_close(socket);
        return NULL;
    }

    return socket;
}


pomelo_platform_udp_t * pomelo_platform_udp_controller_connect(
    pomelo_platform_udp_controller_t * controller,
    pomelo_address_t * address
) {
    assert(controller != NULL);
    assert(address != NULL);

    struct sockaddr_storage addr;
    if (pomelo_address_to_sockaddr(address, &addr) < 0) {
        return NULL;
    }

    pomelo_platform_udp_t * socket =
        pomelo_pool_acquire(controller->socket_pool);
    if (!socket) {
        return NULL;
    }

    // Setup socket
    socket->controller = controller;

    // Setup UDP handle
    uv_udp_t * udp = &socket->uv_udp;
    uv_udp_init(controller->uv_loop, udp);
    udp->data = socket;

    int send_buf_size = POMELO_CLIENT_SOCKET_SNDBUF_SIZE;
    int recv_buf_size = POMELO_CLIENT_SOCKET_RCVBUF_SIZE;

    uv_send_buffer_size((uv_handle_t *) udp, &send_buf_size);
    uv_recv_buffer_size((uv_handle_t *) udp, &recv_buf_size);

    int err;
    
#if UV_UV_CONNECT_AVAILABLE == 1
    // uv_udp_connect is only available after uv version 1.27.0
    err = uv_udp_connect(udp, (struct sockaddr *) &addr);
#else
    struct sockaddr_in bind_addr;
    uv_ip4_addr("0.0.0.0", 0, &bind_addr);
    err = uv_udp_bind(udp, (const struct sockaddr*) &bind_addr, 0);
    if (err == 0) {
        // Copy the target address
        memcpy(&socket->target_addr, &addr, sizeof(struct sockaddr_storage));
    }
#endif

    if (err < 0) {
        pomelo_platform_udp_close(socket);
        return NULL;
    }

    if (uv_udp_recv_start(
        udp,
        pomelo_platform_udp_alloc_callback,
        pomelo_platform_udp_recv_callback
    ) < 0) {
        pomelo_platform_udp_close(socket);
        return NULL;
    }

    socket->node = pomelo_list_push_back(controller->sockets, socket);
    if (!socket->node) {
        pomelo_platform_udp_close(socket);
        return NULL;
    }

    return socket;
}


int pomelo_platform_udp_controller_stop(
    pomelo_platform_udp_controller_t * controller,
    pomelo_platform_udp_t * socket
) {
    (void) controller;
    assert(socket != NULL);

    // Stop receiving and close the handle
    pomelo_platform_udp_close(socket);
    return 0;
}


#ifdef _WIN32
#define BUFFER_LENGTH_TYPE ULONG
#else // Unix
#define BUFFER_LENGTH_TYPE size_t
#endif

int pomelo_platform_udp_controller_send(
    pomelo_platform_udp_controller_t * controller,
    pomelo_platform_udp_t * socket,
    pomelo_address_t * address,
    int nbuffers,
    pomelo_buffer_vector_t * buffers,
    void * callback_data
) {
    assert(controller != NULL);
    assert(socket != NULL);
    assert(buffers != NULL);
    assert(nbuffers > 0);

    struct sockaddr_storage addr;
    if (address != NULL && pomelo_address_to_sockaddr(address, &addr) < 0) {
        return -1;
    }

    pomelo_platform_send_t * send = pomelo_pool_acquire(controller->send_pool);
    if (!send) {
        return -1;
    }

    send->callback_data = callback_data;
    send->socket = socket;
    send->uv_req.data = send;

    uv_buf_t bufs[POMELO_PLATFORM_UDP_MAX_NUMBER_BUF_VECTORS];
    bufs[0].base = (char *) buffers[0].data;
    bufs[0].len = (BUFFER_LENGTH_TYPE) buffers[0].length;

    controller->send_bytes += buffers[0].length;

    if (nbuffers > 1) {
        bufs[1].base = (char *) buffers[1].data;
        bufs[1].len = (BUFFER_LENGTH_TYPE) buffers[1].length;
        controller->send_bytes += buffers[1].length;
    }

    return uv_udp_send(
        &send->uv_req,
        &socket->uv_udp,
        bufs,
        nbuffers,
        address
            ? ((struct sockaddr *) &addr)
#if UV_UV_CONNECT_AVAILABLE == 1
            : NULL,
#else
            : (const struct sockaddr *) &socket->target_addr,
#endif
        pomelo_platform_send_done
    );
}


void pomelo_platform_udp_controller_set_callbacks(
    pomelo_platform_udp_controller_t * controller,
    pomelo_platform_udp_t * socket,
    void * context,
    pomelo_platform_alloc_cb alloc_callback,
    pomelo_platform_recv_cb recv_callback,
    pomelo_platform_send_cb send_callback
) {
    (void) controller;
    assert(socket != NULL);

    socket->alloc_callback = alloc_callback;
    socket->recv_callback = recv_callback;
    socket->send_callback = send_callback;
    socket->context = context;
}


/* -------------------------------------------------------------------------- */
/*                               Private APIs                                 */
/* -------------------------------------------------------------------------- */

void pomelo_platform_udp_alloc_callback(
    uv_handle_t * handle,
    size_t suggested_size,
    uv_buf_t * buf
) {
    (void) suggested_size;

    pomelo_platform_udp_t * socket = handle->data;
    assert(socket != NULL);
    assert(socket->alloc_callback != NULL);

    pomelo_buffer_vector_t buffer;
    memset(&buffer, 0, sizeof(pomelo_buffer_vector_t));

    if (!socket->alloc_callback) {
        // No alloc callback is set
        buf->base = NULL;
        buf->len = 0;
        return;
    }

    socket->alloc_callback(socket->context, &buffer);
    
    buf->base = (char *) buffer.data;
    buf->len = (BUFFER_LENGTH_TYPE) buffer.length;
}


void pomelo_platform_udp_recv_callback(
    uv_udp_t * handle,
    ssize_t nread,
    const uv_buf_t * buf,
    const struct sockaddr * addr,
    unsigned flags
) {
    pomelo_platform_udp_t * socket = handle->data;
    assert(socket != NULL);
    (void) flags;

    pomelo_address_t address;
    int address_valid = addr 
        ? pomelo_address_from_sockaddr(&address, addr) == 0
        : 0;

    pomelo_buffer_vector_t buffer;
    buffer.data = (uint8_t *) buf->base;
    buffer.length = (size_t) nread;

    if (nread > 0) {
        socket->controller->recv_bytes += nread;
    }

    if (socket->recv_callback) {
        socket->recv_callback(
            socket->context,
            address_valid ? &address : NULL,
            &buffer,
            (address_valid && nread > 0) ? 0 : -1
        );
    }
}


void pomelo_platform_send_done(uv_udp_send_t * req, int status) {
    pomelo_platform_send_t * send = req->data;
    pomelo_platform_udp_t * socket = send->socket;
    pomelo_platform_udp_controller_t * controller = socket->controller;

    // Capture the values
    void * callback_data = send->callback_data;
    pomelo_platform_send_cb callback = socket->send_callback;

    // Release the sending pass
    pomelo_pool_release(controller->send_pool, send);

    // Finally, call the callback
    if (callback) {
        callback(socket->context, callback_data, status);
    }
}


void pomelo_platform_udp_on_closed(uv_udp_t * udp) {
    pomelo_platform_udp_t * socket = udp->data;
    pomelo_platform_udp_controller_t * controller = socket->controller;

    pomelo_list_remove(controller->sockets, socket->node);
    pomelo_pool_release(controller->socket_pool, socket);
}


void pomelo_platform_udp_close(pomelo_platform_udp_t * socket) {
    assert(socket != NULL);
    if (socket->closing) {
        return;
    }

    socket->closing = true;
    uv_udp_t * udp = &socket->uv_udp;
    uv_udp_recv_stop(udp);
    uv_close((uv_handle_t *) udp, (uv_close_cb) pomelo_platform_udp_on_closed);
}

