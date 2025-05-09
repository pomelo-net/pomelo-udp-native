# Pomelo Protocol Packets Documentation

## Overview
The Pomelo protocol uses various packet types for connection establishment, maintenance, and data transfer. All packets follow specific formats and encryption rules to ensure secure communication.

## General Packet Structure

### 1. Unencrypted Packets
Only the connection request packet is unencrypted:
```c
struct connection_request_packet {
    uint8_t prefix;                // Must be 0
    char version_info[13];         // "POMELO 1.03\0"
    uint64_t protocol_id;          // Application protocol ID
    uint64_t expire_timestamp;     // Token expiration time
    uint8_t nonce[24];            // Token nonce
    uint8_t token_data[1024];     // Encrypted private token data
};
```

### 2. Encrypted Packets
All other packets follow this general structure:
```c
struct encrypted_packet {
    uint8_t prefix;               // ((num_sequence_bytes<<4) | packet_type)
    uint8_t sequence[1-8];       // Variable length sequence number
    uint8_t encrypted_data[];    // Type-specific encrypted data
    uint8_t hmac[16];           // HMAC of encrypted data
};
```

## Packet Types

### 1. Connection Request (0)
- Unencrypted packet
- Size: 1078 bytes
- Used to initiate connection with server
- Contains connect token data

### 2. Connection Denied (1)
- Encrypted packet
- Size: Minimum 18 bytes (prefix + sequence + HMAC)
- No additional data
- Sent when connection is rejected

### 3. Connection Challenge (2)
- Encrypted packet
- Size: 326 bytes
```c
struct challenge_packet {
    uint64_t challenge_token_sequence;
    uint8_t encrypted_challenge_token[300];
};
```

### 4. Connection Response (3)
- Encrypted packet
- Size: 326 bytes
```c
struct response_packet {
    uint64_t challenge_token_sequence;
    uint8_t encrypted_challenge_token[300];
};
```

### 5. Connection Ping (4)
- Encrypted packet
- Size: 3-19 bytes (variable)
```c
struct ping_packet {
    uint8_t ping_meta;           // Metadata for ping
    uint8_t time[0-8];          // Variable length timestamp
    uint8_t client_id[1-8];     // Variable length client ID
    uint8_t ping_sequence[1-2]; // Variable length sequence
};
```

### 6. Connection Payload (5)
- Encrypted packet
- Size: 1-1200 bytes (variable)
```c
struct payload_packet {
    uint8_t payload_data[1-1200]; // Application data
};
```

### 7. Connection Disconnect (6)
- Encrypted packet
- Size: Minimum 18 bytes (prefix + sequence + HMAC)
- No additional data
- Sent when terminating connection

### 8. Connection Pong (7)
- Encrypted packet
- Size: 4-25 bytes (variable)
```c
struct pong_packet {
    uint8_t pong_encrypted_meta;  // Metadata for pong
    uint8_t ping_sequence[1-2];   // Original ping sequence
    uint8_t ping_recv_time[1-8];  // When ping was received
    uint8_t pong_delta_time[1-8]; // Processing time delta
};
```

## Packet Prefix Byte
The prefix byte contains two pieces of information:
- High 4 bits: Number of sequence number bytes (1-8)
- Low 4 bits: Packet type (0-7)

Example:
```c
prefix = (num_sequence_bytes << 4) | packet_type;
// For 2 sequence bytes and packet type 5:
// prefix = (2 << 4) | 5 = 37
```

## Sequence Numbers
- 64-bit values encoded with variable length (1-8 bytes)
- Written in reverse byte order
- Used for replay protection
- Included in encryption nonce

Example encoding:
```c
// Sequence number 1000 (0x000003E8)
// Requires 2 bytes: 0xE8 0x03
```

## Encryption Process

### 1. Associated Data
```c
struct encryption_associated_data {
    char version_info[13];     // "POMELO 1.03\0"
    uint64_t protocol_id;      // Application protocol ID
    uint8_t prefix;           // Packet prefix byte
};
```

### 2. Encryption Details
- Uses libsodium AEAD (crypto_aead_chacha20poly1305_ietf_encrypt)
- Nonce created from sequence number (padded to 96 bits)
- Different keys for client→server and server→client
- HMAC included in last 16 bytes

## Packet Validation

### 1. Size Validation
- Minimum size: 18 bytes
- Maximum size: 1200 bytes (for payload packets)
- Type-specific size requirements

### 2. Prefix Validation
- Type must be 0-7
- Sequence bytes must be 1-8

### 3. Sequence Validation
- Must be within replay protection window
- Must not be previously received
- Updates replay protection state

### 4. Encryption Validation
- HMAC must be valid
- Decryption must succeed
- Associated data must match
