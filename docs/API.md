# Pomelo UDP Library API Documentation

## Overview
Pomelo UDP is a connection-oriented UDP networking library that provides reliable and secure communication with built-in connection management, encryption, and delivery guarantees.

## Core Concepts

### Socket
The primary interface for network communication. Can operate in either client or server mode.

### Session
Represents an established connection between peers with:
- Unique client ID
- Multiple channels
- RTT statistics
- Connection state

### Channel
A logical communication path with configurable delivery modes:
- `RELIABLE`: Guaranteed delivery and ordering
- `SEQUENCED`: Ordered delivery without guarantees
- `UNRELIABLE`: Best-effort delivery

## API Reference

### Socket Creation and Management

```c
// Create a server socket
pomelo_socket_t* pomelo_server_create(
    pomelo_server_config_t* config
);

// Create a client socket
pomelo_socket_t* pomelo_client_create(
    pomelo_client_config_t* config
);

// Start the socket
int pomelo_socket_start(pomelo_socket_t* socket);

// Stop the socket
void pomelo_socket_stop(pomelo_socket_t* socket);

// Destroy the socket
void pomelo_socket_destroy(pomelo_socket_t* socket);
```

### Configuration Structures

```c
struct pomelo_server_config_s {
    pomelo_allocator_t* allocator;      // Memory allocator
    uint64_t protocol_id;               // Protocol identifier
    uint8_t private_key[PRIVATE_KEY_BYTES]; // Server private key
    size_t max_clients;                 // Maximum concurrent clients
    size_t nchannels;                   // Number of channels
    pomelo_channel_mode* channel_modes; // Channel configuration
};

struct pomelo_client_config_s {
    pomelo_allocator_t* allocator;      // Memory allocator
    size_t nchannels;                   // Number of channels
    pomelo_channel_mode* channel_modes; // Channel configuration
};
```

### Session Management

```c
// Get session statistics
void pomelo_session_get_rtt(
    pomelo_session_t* session,
    uint64_t* mean,        // Mean RTT
    uint64_t* variance     // RTT variance
);

// Configure channel mode
int pomelo_session_set_channel_mode(
    pomelo_session_t* session,
    size_t channel_index,
    pomelo_channel_mode mode
);

// Send data through a channel
int pomelo_session_send(
    pomelo_session_t* session,
    size_t channel_index,
    const void* data,
    size_t length
);

// Disconnect a session
void pomelo_session_disconnect(
    pomelo_session_t* session
);
```

### Connect Token Management

```c
// Generate a connect token
int pomelo_generate_connect_token(
    pomelo_connect_token_config_t* config,
    uint8_t* token_buffer
);

// Decode a connect token
int pomelo_decode_connect_token(
    const uint8_t* token_buffer,
    pomelo_connect_token_info_t* info
);
```

### Event Callbacks

```c
// Socket event callbacks
struct pomelo_socket_callbacks_s {
    void (*on_started)(pomelo_socket_t* socket);
    void (*on_stopped)(pomelo_socket_t* socket);
    void (*on_session_connected)(pomelo_session_t* session);
    void (*on_session_disconnected)(pomelo_session_t* session);
    void (*on_message_received)(
        pomelo_session_t* session,
        size_t channel_index,
        const void* data,
        size_t length
    );
};
```

## Plugin System

### Plugin Interface

```c
struct pomelo_plugin_s {
    // Plugin lifecycle
    void (*on_load)(pomelo_plugin_t* plugin);
    void (*on_unload)(pomelo_plugin_t* plugin);
    
    // Socket events
    void (*on_socket_created)(pomelo_socket_t* socket);
    void (*on_socket_destroyed)(pomelo_socket_t* socket);
    
    // Session events
    void (*on_session_created)(pomelo_session_t* session);
    void (*on_session_destroyed)(pomelo_session_t* session);
};
```

## Thread Safety

### Thread-Safe Operations
- Socket creation/destruction
- Session management
- Message sending/receiving
- Configuration changes

### Non-Thread-Safe Operations
- Socket start/stop
- Plugin loading/unloading
- Callback registration

## Error Handling

### Error Codes
```c
enum pomelo_error_e {
    POMELO_ERROR_OK = 0,
    POMELO_ERROR_INVALID_ARGUMENT = -1,
    POMELO_ERROR_OUT_OF_MEMORY = -2,
    POMELO_ERROR_CONNECTION_FAILED = -3,
    POMELO_ERROR_TIMEOUT = -4,
    // ... other error codes
};
```

## Best Practices

### Socket Management
1. Always check return values for error conditions
2. Configure appropriate timeouts
3. Handle disconnections gracefully
4. Clean up resources properly

### Message Sending
1. Choose appropriate channel modes
2. Handle send failures
3. Monitor RTT statistics
4. Implement proper flow control

### Resource Management
1. Use appropriate buffer sizes
2. Monitor memory usage
3. Handle resource exhaustion
4. Implement proper cleanup

### Security
1. Protect private keys
2. Validate connect tokens
3. Monitor for suspicious activity
4. Implement proper access control

## Example Usage

### Server Implementation
```c
// Initialize server configuration
pomelo_server_config_t config = {0};
config.protocol_id = MY_PROTOCOL_ID;
config.max_clients = 32;
config.nchannels = 2;
config.channel_modes[0] = POMELO_CHANNEL_MODE_RELIABLE;
config.channel_modes[1] = POMELO_CHANNEL_MODE_SEQUENCED;

// Create and start server
pomelo_socket_t* server = pomelo_server_create(&config);
pomelo_socket_start(server);

// Handle events through callbacks
// Clean up when done
pomelo_socket_stop(server);
pomelo_socket_destroy(server);
```

### Client Implementation
```c
// Initialize client configuration
pomelo_client_config_t config = {0};
config.nchannels = 2;
config.channel_modes[0] = POMELO_CHANNEL_MODE_RELIABLE;
config.channel_modes[1] = POMELO_CHANNEL_MODE_SEQUENCED;

// Create and connect client
pomelo_socket_t* client = pomelo_client_create(&config);
pomelo_socket_start(client);

// Send messages through channels
pomelo_session_send(session, 0, data, length);

// Clean up when done
pomelo_socket_stop(client);
pomelo_socket_destroy(client);
```
```

This documentation provides a comprehensive overview of the public API. Let me know if you'd like me to expand on any particular section or add more specific details.