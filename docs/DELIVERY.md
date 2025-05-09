# Pomelo Delivery Layer Documentation

## Overview
The delivery layer provides reliable and ordered packet delivery over UDP. It handles packet fragmentation, reassembly, and ensures delivery guarantees through different delivery modes.

## Core Components

### 1. Transporter
The main interface for sending and receiving data. It manages:
- Connection endpoints
- Delivery contexts
- Packet routing between endpoints

### 2. Endpoint
Represents a communication endpoint that:
- Handles incoming and outgoing parcels
- Manages multiple delivery buses
- Coordinates checksum verification

### 3. Bus
A logical channel for packet delivery that:
- Handles reliable and sequenced delivery modes
- Manages packet ordering and acknowledgments
- Tracks incomplete parcels and handles retransmission

### 4. Parcel
A complete message that may be split into multiple fragments. Features:
- Reference counting for memory management
- Fragment management
- Checksum verification

### 5. Fragment
The smallest unit of transmission containing:
- Metadata header
- Payload data
- Delivery type information

## Delivery Modes

### Reliable Mode
- Guarantees delivery and ordering
- Uses acknowledgments and retransmission
- Suitable for critical data

### Sequenced Mode
- Maintains ordering but doesn't guarantee delivery
- Drops outdated packets
- Ideal for real-time data where latest state matters

### Unreliable Mode
- No delivery or ordering guarantees
- Packets may be dropped or arrive out of order
- Lowest overhead delivery method
- Best for high-frequency updates where occasional loss is acceptable
- Typical use cases include position updates, telemetry data


## Fragment Format

### Metadata Layout
Field Layout:
Offset | Field Name | Size | Value Range
---|------------------|---------------|-------------
0 | meta_byte | 1 byte | [0-255]
1 | bus_index | 1-2 bytes | [0-65535]
+1 | fragment_index | 1-2 bytes | [0-65535]
+1 | total_fragments | 1-2 bytes | [0-65535]
+1 | sequence | 1-8 bytes | [0-2^64-1]

### Meta Byte Structure
Bits  | Field Name            | Description
------|----------------------|-------------
7-6   | fragment_type        | Type of fragment (2 bits)
5     | bus_index_bytes      | Size of bus_index field
4     | fragment_index_bytes | Size of fragment_index field
3     | total_fragments_bytes| Size of total_fragments field
2-0   | sequence_bytes       | Size of sequence field

## Flow Control

### Sending Process
1. Message is wrapped in a parcel
2. Parcel is split into fragments if needed
3. Fragments are assigned sequence numbers
4. Metadata is attached to each fragment
5. Fragments are transmitted through appropriate bus

### Receiving Process
1. Fragments are collected and validated
2. Fragments are reassembled into parcels
3. Checksums are verified
4. Complete parcels are delivered to the application
5. Acknowledgments are sent for reliable delivery

## Checksum System
- Validates data integrity
- Supports both synchronous and asynchronous verification
- Can be customized per endpoint

## Resource Management
- Reference counting for parcels
- Pool-based allocation for fragments
- Automatic cleanup of incomplete parcels
- Timer-based expiration of pending operations

## Error Handling
- Fragment validation
- Sequence number verification
- Checksum validation
- Timeout management
- Resource exhaustion prevention

## Best Practices
1. Choose appropriate delivery mode based on data requirements
2. Monitor bus performance metrics
3. Configure appropriate timeout values
4. Handle resource cleanup properly
5. Implement proper error handling

## Thread Safety
The delivery layer supports concurrent operations through:
- Thread-safe endpoint operations
- Synchronized resource pools
- Atomic reference counting
- Safe state transitions
