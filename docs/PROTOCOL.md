# Pomelo Protocol Layer Documentation

## Overview
The protocol layer implements a secure, connection-oriented protocol over UDP. It handles connection establishment, peer management, packet encryption, and connection state management for both client and server modes.

## Core Components

### Socket
The base component for network communication that:
- Supports both client and server modes
- Manages packet encryption/decryption
- Handles connection state
- Processes incoming/outgoing packets
- Manages peer connections

#### Socket States
STOPPED -> RUNNING -> STOPPING -> STOPPED

#### Socket Modes
- `SERVER`: Listens for incoming connections
- `CLIENT`: Initiates connections to servers

### Peer
Represents a connected endpoint with:
- Connection state management
- RTT (Round Trip Time) tracking
- Packet sequencing
- Connection timeout monitoring
- Send/receive pass queues

### Pass System
Handles packet processing through:
- `send_pass`: Manages outgoing packet preparation and encryption
- `recv_pass`: Handles incoming packet validation and decryption

### Clock
Provides time synchronization features:
- Network time synchronization
- RTT calculations
- Timeout management
- Drift compensation

## Packet Types

### Connection Establishment
1. `REQUEST`: Initial connection request
2. `CHALLENGE`: Server's challenge response
3. `RESPONSE`: Client's challenge response
4. `DENIED`: Connection rejection

### Connection Maintenance
1. `PING`: Connection keepalive and RTT measurement
2. `PONG`: Ping response
3. `DISCONNECT`: Graceful connection termination
4. `PAYLOAD`: Application data

## Connection Flow

### Client Connection Process
1. Send connection request with token
2. Receive challenge from server
3. Send challenge response
4. Establish connection or handle denial
5. Begin ping/pong maintenance

### Server Connection Process
1. Receive connection request
2. Validate token
3. Send challenge
4. Validate challenge response
5. Accept or deny connection
6. Begin connection maintenance

## Security Features

### Packet Encryption
- Encrypted packet validation
- Key management per connection
- Challenge-response security
- Token-based authentication

### Connection Token
- Time-based expiration
- Server address validation
- Protocol version verification
- Client identification

## Timing and Synchronization

### Frequency Constants
```c
PING_FREQUENCY_HZ: 10 Hz
PONG_REPLY_FREQUENCY_HZ: 20 Hz
CONNECTION_REQUEST_RESPONSE_FREQUENCY_HZ: 10 Hz
DISCONNECT_FREQUENCY_HZ: 10 Hz
```

### Clock Synchronization
- Network time offset calculation
- RTT-based synchronization
- Jitter compensation
- Drift correction

## Resource Management

### Memory Management
- Packet pools
- Pass pools
- Buffer management
- Reference counting

### Connection Management
- Maximum client limits
- Connection timeouts
- Resource cleanup
- State transitions

## Error Handling

### Connection Failures
- Token validation failures
- Challenge failures
- Timeout handling
- Encryption errors

### Packet Processing
- Invalid packet detection
- Sequence validation
- Encryption validation
- Size validation

## Best Practices

### Server Implementation
1. Configure appropriate client limits
2. Implement proper token validation
3. Monitor connection states
4. Handle disconnections gracefully
5. Manage resource usage

### Client Implementation
1. Handle connection retries appropriately
2. Implement proper token handling
3. Monitor connection health
4. Handle disconnections gracefully
5. Manage reconnection logic

## Thread Safety
The protocol layer ensures thread safety through:
- Atomic operations
- State synchronization
- Resource locking
- Safe packet processing

## Performance Considerations

### Packet Processing
- Efficient encryption/decryption
- Optimized packet validation
- Pool-based resource management
- Minimal copying

### Connection Management
- Fast peer lookup
- Efficient state transitions
- Optimized timeout handling
- Quick resource cleanup

## Debugging Support

### Statistics
- Valid/invalid packet counts
- Connection states
- RTT measurements
- Resource usage

### Debug Features
- Packet validation logging
- Connection state tracking
- Resource usage monitoring
- Error tracking
