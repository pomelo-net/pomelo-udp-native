# Pomelo UDP Plugin Development Guide

## Overview
Pomelo UDP supports a plugin system that allows you to extend functionality and handle network events. Plugins can intercept socket events, manage sessions, and implement custom behaviors.

## Plugin Structure

### Basic Plugin Template
```c
#include "pomelo/plugin.h"

// Plugin data structure
typedef struct my_plugin_data_s {
    // Custom plugin data
} my_plugin_data_t;

// Callback implementations
static void on_unload(pomelo_plugin_t* plugin) {
    my_plugin_data_t* data = plugin->get_data(plugin);
    // Cleanup plugin data
}

static void on_socket_created(pomelo_plugin_t* plugin, pomelo_socket_t* socket) {
    // Handle socket creation
}

static void on_socket_destroyed(pomelo_plugin_t* plugin, pomelo_socket_t* socket) {
    // Handle socket destruction
}

// Plugin entry point
POMELO_PLUGIN_ENTRY_REGISTER(register_plugin) {
    // Configure plugin callbacks
    plugin->configure_callbacks(
        plugin,
        on_unload,                    // Unload callback
        on_socket_created,            // Socket created
        on_socket_destroyed,          // Socket destroyed
        on_socket_listening,          // Server listening
        on_socket_connecting,         // Client connecting
        on_socket_stopped,            // Socket stopped
        on_session_disconnect,        // Session disconnect
        on_session_get_rtt,          // Get session RTT
        on_session_set_mode,         // Set channel mode
        on_session_send              // Send message
    );

    // Initialize plugin data
    my_plugin_data_t* data = malloc(sizeof(my_plugin_data_t));
    plugin->set_data(plugin, data);
}
```

## Plugin Callbacks

### Socket Lifecycle
```c
// Called when a socket is created
void on_socket_created(
    pomelo_plugin_t* plugin,
    pomelo_socket_t* socket
);

// Called when socket starts listening (server mode)
void on_socket_listening(
    pomelo_plugin_t* plugin,
    pomelo_socket_t* socket,
    pomelo_address_t* address
);

// Called when socket starts connecting (client mode)
void on_socket_connecting(
    pomelo_plugin_t* plugin,
    pomelo_socket_t* socket,
    uint8_t* connect_token
);

// Called when socket is stopped
void on_socket_stopped(
    pomelo_plugin_t* plugin,
    pomelo_socket_t* socket
);

// Called when socket is destroyed
void on_socket_destroyed(
    pomelo_plugin_t* plugin,
    pomelo_socket_t* socket
);
```

### Session Management
```c
// Handle session disconnection
void on_session_disconnect(
    pomelo_plugin_t* plugin,
    pomelo_session_t* session
);

// Get session RTT information
void on_session_get_rtt(
    pomelo_plugin_t* plugin,
    pomelo_session_t* session,
    uint64_t* mean,
    uint64_t* variance
);

// Set channel mode
int on_session_set_mode(
    pomelo_plugin_t* plugin,
    pomelo_session_t* session,
    size_t channel_index,
    pomelo_channel_mode mode
);

// Handle message sending
void on_session_send(
    pomelo_plugin_t* plugin,
    pomelo_session_t* session,
    size_t channel_index,
    pomelo_message_t* message
);
```

## Plugin API

### Plugin Data Management
```c
// Set plugin data
plugin->set_data(plugin, void* data);

// Get plugin data
void* data = plugin->get_data(plugin);
```

### Socket Operations
```c
// Get number of channels
size_t nchannels = plugin->socket_get_nchannels(plugin, socket);

// Get channel mode
pomelo_channel_mode mode = plugin->socket_get_channel_mode(
    plugin, socket, channel_index
);

// Attach/detach socket
plugin->socket_attach(plugin, socket);
plugin->socket_detach(plugin, socket);

// Get socket time
uint64_t time = plugin->socket_time(plugin, socket);
```

### Session Operations
```c
// Create session
pomelo_session_t* session = plugin->session_create(
    plugin, socket, client_id, address
);

// Destroy session
plugin->session_destroy(plugin, session);

// Set/get session private data
plugin->session_set_private(plugin, session, data);
void* data = plugin->session_get_private(plugin, session);

// Receive message
plugin->session_receive(plugin, session, channel_index, message);
```

### Message Handling
```c
// Create new message
pomelo_message_t* msg = plugin->message_acquire(plugin);

// Read/write message data
plugin->message_write(plugin, msg, length, buffer);
plugin->message_read(plugin, msg, length, buffer);

// Get message length
size_t len = plugin->message_length(plugin, msg);
```

## Thread Safety

### Thread-Safe Operations
- Plugin data access (get_data/set_data)
- Session private data access
- Message operations
- Executor operations

### Non-Thread-Safe Operations
- Plugin registration
- Socket attachment/detachment
- Session creation/destruction

## Best Practices

### Resource Management
1. Always clean up plugin data in on_unload
2. Release acquired messages when done
3. Clean up session private data
4. Use proper thread synchronization

### Error Handling
1. Check return values from API calls
2. Handle memory allocation failures
3. Validate parameters
4. Handle network errors gracefully

### Performance
1. Minimize work in callbacks
2. Use thread-safe executor for heavy tasks
3. Avoid blocking operations
4. Pool resources when appropriate

## Example Plugin Implementation

### Statistics Plugin
```c
typedef struct stats_plugin_s {
    uint64_t messages_sent;
    uint64_t messages_received;
    uint64_t bytes_sent;
    uint64_t bytes_received;
} stats_plugin_t;

static void on_session_send(
    pomelo_plugin_t* plugin,
    pomelo_session_t* session,
    size_t channel_index,
    pomelo_message_t* message
) {
    stats_plugin_t* stats = plugin->get_data(plugin);
    stats->messages_sent++;
    stats->bytes_sent += plugin->message_length(plugin, message);
}

static void on_unload(pomelo_plugin_t* plugin) {
    stats_plugin_t* stats = plugin->get_data(plugin);
    printf("Stats: Sent %lu messages (%lu bytes)\n",
           stats->messages_sent, stats->bytes_sent);
    free(stats);
}

POMELO_PLUGIN_ENTRY_REGISTER(register_stats_plugin) {
    stats_plugin_t* stats = calloc(1, sizeof(stats_plugin_t));
    plugin->set_data(plugin, stats);
    
    plugin->configure_callbacks(
        plugin,
        on_unload,
        NULL,  // on_socket_created
        NULL,  // on_socket_destroyed
        NULL,  // on_socket_listening
        NULL,  // on_socket_connecting
        NULL,  // on_socket_stopped
        NULL,  // on_session_disconnect
        NULL,  // on_session_get_rtt
        NULL,  // on_session_set_mode
        on_session_send
    );
}
```
