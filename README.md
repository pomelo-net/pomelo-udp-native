# Pomelo UDP Native

A high-performance UDP networking framework for game development, backed by libuv and libsodium.

## Overview

Pomelo UDP Native is a robust networking framework designed specifically for game development, providing reliable/unreliable UDP communication with features like:

- Reliable and unreliable message delivery
- Connection management
- Plugin system
- Cross-platform support
- Built-in security features

## Features

- **Reliable/Unreliable/Sequenced UDP**: Implements reliable/unreliable/sequenced message delivery over UDP
- **Plugin Architecture**: Extensible plugin system for custom functionality
- **Security**: Built-in encryption and token-based authentication
- **Cross-Platform**: Supports multiple platforms through libuv
- **Performance**: Optimized for low-latency game networking

## Dependencies

- [libuv](https://github.com/libuv/libuv): A multi-platform support library with a focus on asynchronous I/O
- [libsodium](https://github.com/jedisct1/libsodium): A modern, portable, and easy to use crypto library

## Project Structure

```
pomelo-udp-native/
├── include/          # Public headers
├── src/             # Source files
├── deps/            # Dependencies (libuv, libsodium)
├── example/         # Example applications
├── test/            # Test suite
├── docs/            # Documentation
└── build/           # Build output directory
```

## Architecture
![Architecture](docs/contents/architecture.svg)


## Documentation

Detailed documentation is available in the `docs/` directory:

- [API Documentation](docs/API.md)
- [Protocol Specification](docs/PROTOCOL.md)
- [Plugin System](docs/PLUGIN.md)
- [Platform Support](docs/PLATFORM.md)
- [Delivery System](docs/DELIVERY.md)
- [Connect Token](docs/CONNECT_TOKEN.md)
- [Packets](docs/PACKETS.md)
- [Standards](docs/STANDARD.md)

## Examples

The [`example`](example/) directory contains sample applications demonstrating key features:

- [`server.c`](example/server.c) - A UDP server implementation showing connection handling, message processing, and basic game state management
- [`client.c`](example/client.c) - A client application demonstrating connection establishment, message sending/receiving, and game state updates


## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.
