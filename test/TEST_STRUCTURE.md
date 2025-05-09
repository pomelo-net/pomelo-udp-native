# Pomelo UDP Native Test Structure

This document outlines the test structure for the Pomelo UDP Native library.

## Test Categories

### 1. API Tests (`api-test/`)
- Tests for the public API interface
- Connection between API client & server
- API usage examples and validation

### 2. Base Tests (`base-test/`)
- Core data structures and utilities
  - Address handling
  - Memory allocator
  - Array implementation
  - Codec functionality
  - List & Unrolled list
  - Map implementation
  - Payload handling
  - Memory pool
  - Reference counting

### 3. Codec Tests (`codec-test/`)
- Tests for encoding and decoding functionality
- Checksum verification
- Data serialization/deserialization

### 4. Delivery Tests (`delivery-test/`)
- Message delivery mechanisms
- Transfer of large messages through delivery layer
- Reliability and sequencing tests
- Fragment handling

### 5. Platform Tests (`platform-test/`)
- Platform-specific implementations
  - UV & Poll platforms
  - Task scheduling
  - Timer functionality
  - UDP Socket operations

### 6. Plugin Tests (`plugin-test/`)
- Plugin system functionality
- Loading and registering demo plugins
- Plugin lifecycle management

### 7. Protocol Tests (`protocol-test/`)
- Protocol implementation verification
- Client with server simulator
- Server with client simulator
- Protocol state machine tests

### 8. Utils Tests (`utils-test/`)
- Utility functions and helpers
- Common functionality used across the codebase

### 9. WebRTC Plugin Tests (`webrtc-plugin-test/`)
- WebRTC integration tests
- Loading and running WebRTC plugin
- WebRTC-specific functionality

## Test Framework

All tests use the framework defined in `pomelo-test.h`, which provides common testing utilities and macros for consistent test implementation across all categories.
