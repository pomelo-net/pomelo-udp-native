cmake_minimum_required(VERSION 3.10)

project(pomelo VERSION 0.0.1)
set(CMAKE_C_STANDARD 11)

option(POMELO_MULTI_THREAD "Multi-thread mode" ON)

set(POMELO_BASE pomelo-base)
set(POMELO_PROTOCOL pomelo-protocol)
set(POMELO_ADAPTER_DEFAULT pomelo-adapter-default)
set(POMELO_CODEC pomelo-codec)
set(POMELO_UTILS pomelo-utils)
set(POMELO_DELIVERY pomelo-delivery)
set(POMELO_PLATFORM_UV pomelo-platform-uv)
set(POMELO_PLATFORM_POLL pomelo-platform-poll)
set(POMELO_API pomelo-api)
set(POMELO_GENERATOR pomelo-generator)
set(POMELO_EXAMPLE_CLIENT pomelo-example-client)
set(POMELO_EXAMPLE_SERVER pomelo-example-server)
set(POMELO_EXAMPLE_SHARED pomelo-example-shared)

set(POMELO_TEST_BASE pomelo-test-base)
set(POMELO_TEST_PLATFORM_UV pomelo-test-platform-uv)
set(POMELO_TEST_PLATFORM_POLL pomelo-test-platform-poll)
set(POMELO_TEST_PROTOCOL pomelo-test-protocol)
set(POMELO_TEST_PROTOCOL_UNENCRYPTED pomelo-test-protocol-unencrypted)
set(POMELO_TEST_PROTOCOL_CLIENT pomelo-test-protocol-client)
set(POMELO_TEST_PROTOCOL_SERVER pomelo-test-protocol-server)
set(POMELO_TEST_DELIVERY pomelo-test-delivery)
set(POMELO_TEST_API_UV pomelo-test-api-uv)
set(POMELO_TEST_API_POLL pomelo-test-api-poll)
set(POMELO_TEST_PLUGIN pomelo-test-plugin)
set(POMELO_TEST_DEMO_PLUGIN pomelo-test-demo-plugin)
set(POMELO_TEST_WEBRTC_PLUGIN pomelo-test-webrtc-plugin)


# Dependency paths
set(LIB_UV_PATH deps/libuv)
set(LIB_SODIUM_PATH deps/libsodium)
set(LIB_SODIUM_SRC_PATH ${LIB_SODIUM_PATH}/src/libsodium)

# Include paths
set(INCLUDE_UV ${LIB_UV_PATH}/include)
set(INCLUDE_SODIUM
    ${LIB_SODIUM_SRC_PATH}/include
    ${LIB_SODIUM_SRC_PATH}/include/sodium)
set(INCLUDE_POMELO include src)
set(INCLUDE_POMELO_TEST ${INCLUDE_POMELO} test)

# Libuv options
set(LIBUV_BUILD_SHARED OFF)

# libuv
add_subdirectory(deps/libuv)

# Sources
set(SRC_INCLUDE
    include/pomelo/platforms/platform-uv.h
    include/pomelo/statistic/statistic-allocator.h
    include/pomelo/statistic/statistic-api.h
    include/pomelo/statistic/statistic-buffer.h
    include/pomelo/statistic/statistic-delivery.h
    include/pomelo/statistic/statistic-platform.h
    include/pomelo/statistic/statistic-protocol.h
    include/pomelo/address.h
    include/pomelo/allocator.h
    include/pomelo/api.h
    include/pomelo/common.h
    include/pomelo/constants.h
    include/pomelo/errno.h
    include/pomelo/platform.h
    include/pomelo/plugin.h
    include/pomelo/statistic.h
    include/pomelo/token.h
    include/pomelo.h
)

set(SRC_API
    src/api/builtin/builtin.h
    src/api/builtin/channel.c
    src/api/builtin/channel.h
    src/api/builtin/delivery.c
    src/api/builtin/protocol.c
    src/api/builtin/session.c
    src/api/builtin/session.h
    src/api/plugin/channel.c
    src/api/plugin/channel.h
    src/api/plugin/loader.c
    src/api/plugin/manager.c
    src/api/plugin/manager.h
    src/api/plugin/message.c
    src/api/plugin/message.h
    src/api/plugin/plugin.c
    src/api/plugin/plugin.h
    src/api/plugin/session.c
    src/api/plugin/session.h
    src/api/plugin/socket.c
    src/api/plugin/socket.h
    src/api/plugin/token.c
    src/api/plugin/token.h
    src/api/channel.c
    src/api/channel.h
    src/api/context.c
    src/api/context.h
    src/api/message.c
    src/api/message.h
    src/api/session.c
    src/api/session.h
    src/api/socket.c
    src/api/socket.h
)

set(SRC_BASE
    ${SRC_INCLUDE}
    src/base/address.c
    src/base/allocator.c
    src/base/buffer.c
    src/base/buffer.h
    src/base/constants.h
    src/base/extra.h
    src/base/packet.c
    src/base/packet.h
    src/base/payload.c
    src/base/payload.h
    src/base/ref.c
    src/base/ref.h
    src/base/rtt.c
    src/base/rtt.h
)

set(SRC_CODEC
    src/codec/checksum.c
    src/codec/checksum.h
    src/codec/codec.c
    src/codec/codec.h
    src/codec/packed.c
    src/codec/packed.h
    src/codec/packet.c
    src/codec/packet.h
    src/codec/token.c
    src/codec/token.h
)

set(SRC_DELIVERY
    src/delivery/bus.c
    src/delivery/bus.h
    src/delivery/checksum.c
    src/delivery/checksum.h
    src/delivery/commands.c
    src/delivery/commands.h
    src/delivery/context.c
    src/delivery/context.h
    src/delivery/endpoint.c
    src/delivery/endpoint.h
    src/delivery/fragment.c
    src/delivery/fragment.h
    src/delivery/parcel.c
    src/delivery/parcel.h
    src/delivery/transporter.c
    src/delivery/transporter.h
)

set(SRC_GENERATOR src/generator/generator.c)

set(SRC_PLATFORM_COMMON
    src/platform/common/platform-time.c
    src/platform/common/platform-timer.c
    src/platform/common/platform-timer.h
    src/platform/common/platform-udp.c
    src/platform/common/platform-udp.h
)

set(SRC_PLATFORM_UV
    ${SRC_PLATFORM_COMMON}
    src/platform/uv/platform-task-deferred.c
    src/platform/uv/platform-task-deferred.h
    src/platform/uv/platform-task-group.c
    src/platform/uv/platform-task-group.h
    src/platform/uv/platform-task-main.c
    src/platform/uv/platform-task-main.h
    src/platform/uv/platform-task-worker.c
    src/platform/uv/platform-task-worker.h
    src/platform/uv/platform-uv.c
    src/platform/uv/platform-uv.h
    src/platform/platform.h
)

set(SRC_PLATFORM_POLL
    ${SRC_PLATFORM_COMMON}
    src/platform/poll/platform-poll.c
    src/platform/poll/platform-poll.h
    src/platform/poll/platform-task.c
    src/platform/poll/platform-task.h
    src/platform/platform.h
)

set(SRC_PROTOCOL
    src/protocol/adapter.c
    src/protocol/client.c
    src/protocol/client.h
    src/protocol/clock.c
    src/protocol/clock.h
    src/protocol/context.c
    src/protocol/context.h
    src/protocol/emitter.c
    src/protocol/emitter.h
    src/protocol/pass.c
    src/protocol/pass.h
    src/protocol/peer.c
    src/protocol/peer.h
    src/protocol/protocol.h
    src/protocol/server.c
    src/protocol/server.h
    src/protocol/socket.c
    src/protocol/socket.h
)

set(SRC_ADAPTER_BASE src/adapter/adapter.c src/adapter/adapter.h)
set(SRC_ADAPTER_DEFAULT src/adapter/default.c src/adapter/default.h)

set(SRC_UTILS
    src/utils/args.c
    src/utils/args.h
    src/utils/array.c
    src/utils/array.h
    src/utils/atomic.c
    src/utils/atomic.h
    src/utils/list.c
    src/utils/list.h
    src/utils/macro.h
    src/utils/map.c
    src/utils/map.h
    src/utils/mutex.c
    src/utils/mutex.h
    src/utils/pool.c
    src/utils/pool.h
    src/utils/sampling.c
    src/utils/sampling.h
)

# Common test source
set(SRC_TEST_BASE
    test/base-test/address-test.c
    test/base-test/allocator-test.c
    test/base-test/array-test.c
    test/base-test/base-test.c
    test/base-test/codec-test.c
    test/base-test/list-test.c
    test/base-test/unrolled-list-test.c
    test/base-test/map-test.c
    test/base-test/payload-test.c
    test/base-test/pool-test.c
    test/base-test/ref-test.c
)

set(SRC_TEST_PLATFORM_UV
    test/platform-test/platform-test.c
    test/platform-test/platform-test.h
    test/platform-test/platform-uv.c
)

set(SRC_TEST_PLATFORM_POLL
    test/platform-test/platform-poll.c
    test/platform-test/platform-test.c
    test/platform-test/platform-test.h
)

set(SRC_TEST_PROTOCOL test/protocol-test/protocol-test.c)
set(SRC_TEST_PROTOCOL_UNENCRYPTED
    ${SRC_TEST_PROTOCOL}
    ${SRC_ADAPTER_BASE}
    test/protocol-test/adapter-unencrypted.c
    test/protocol-test/adapter-unencrypted.h
)
set(SRC_TEST_PROTOCOL_CLIENT
    ${SRC_ADAPTER_BASE}
    test/protocol-test/adapter-simulator.c
    test/protocol-test/adapter-simulator.h
    test/protocol-test/client-test.c
)
set(SRC_TEST_PROTOCOL_SERVER
    ${SRC_ADAPTER_BASE}
    test/protocol-test/adapter-simulator.c
    test/protocol-test/adapter-simulator.h
    test/protocol-test/server-test.c
)


set(SRC_TEST_DELIVERY test/delivery-test/delivery-test.c)
set(SRC_TEST_API_UV
    test/api-test/api-test.c
    test/platform-test/platform-uv.c
)
set(SRC_TEST_API_POLL
    test/api-test/api-test.c
    test/platform-test/platform-poll.c
)
set(SRC_TEST_PLUGIN test/plugin-test/plugin-test.c)
set(SRC_TEST_DEMO_PLUGIN test/plugin-test/demo-plugin.c)
set(SRC_TEST_WEBRTC_PLUGIN test/webrtc-plugin-test/webrtc-plugin-test.c)

set(SRC_SODIUM
    ${LIB_SODIUM_SRC_PATH}/crypto_aead/aegis128l/aead_aegis128l.c
    ${LIB_SODIUM_SRC_PATH}/crypto_aead/aegis128l/aegis128l_aesni.c
    ${LIB_SODIUM_SRC_PATH}/crypto_aead/aegis128l/aegis128l_armcrypto.c
    ${LIB_SODIUM_SRC_PATH}/crypto_aead/aegis128l/aegis128l_soft.c
    ${LIB_SODIUM_SRC_PATH}/crypto_aead/aegis256/aead_aegis256.c
    ${LIB_SODIUM_SRC_PATH}/crypto_aead/aegis256/aegis256_armcrypto.c
    ${LIB_SODIUM_SRC_PATH}/crypto_aead/aegis256/aegis256_aesni.c
    ${LIB_SODIUM_SRC_PATH}/crypto_aead/aegis256/aegis256_soft.c
    ${LIB_SODIUM_SRC_PATH}/crypto_aead/aes256gcm/aesni/aead_aes256gcm_aesni.c
    ${LIB_SODIUM_SRC_PATH}/crypto_aead/chacha20poly1305/aead_chacha20poly1305.c
    ${LIB_SODIUM_SRC_PATH}/crypto_aead/xchacha20poly1305/aead_xchacha20poly1305.c
    ${LIB_SODIUM_SRC_PATH}/crypto_auth/crypto_auth.c
    ${LIB_SODIUM_SRC_PATH}/crypto_auth/hmacsha256/auth_hmacsha256.c
    ${LIB_SODIUM_SRC_PATH}/crypto_auth/hmacsha512/auth_hmacsha512.c
    ${LIB_SODIUM_SRC_PATH}/crypto_auth/hmacsha512256/auth_hmacsha512256.c
    ${LIB_SODIUM_SRC_PATH}/crypto_box/crypto_box.c
    ${LIB_SODIUM_SRC_PATH}/crypto_box/crypto_box_easy.c
    ${LIB_SODIUM_SRC_PATH}/crypto_box/crypto_box_seal.c
    ${LIB_SODIUM_SRC_PATH}/crypto_box/curve25519xsalsa20poly1305/box_curve25519xsalsa20poly1305.c
    ${LIB_SODIUM_SRC_PATH}/crypto_core/ed25519/ref10/ed25519_ref10.c
    ${LIB_SODIUM_SRC_PATH}/crypto_core/hchacha20/core_hchacha20.c
    ${LIB_SODIUM_SRC_PATH}/crypto_core/hsalsa20/core_hsalsa20.c
    ${LIB_SODIUM_SRC_PATH}/crypto_core/hsalsa20/ref2/core_hsalsa20_ref2.c
    ${LIB_SODIUM_SRC_PATH}/crypto_core/salsa/ref/core_salsa_ref.c
    ${LIB_SODIUM_SRC_PATH}/crypto_core/softaes/softaes.c
    ${LIB_SODIUM_SRC_PATH}/crypto_generichash/blake2b/generichash_blake2.c
    ${LIB_SODIUM_SRC_PATH}/crypto_generichash/blake2b/ref/blake2b-compress-avx2.c
    ${LIB_SODIUM_SRC_PATH}/crypto_generichash/blake2b/ref/blake2b-compress-ref.c
    ${LIB_SODIUM_SRC_PATH}/crypto_generichash/blake2b/ref/blake2b-compress-sse41.c
    ${LIB_SODIUM_SRC_PATH}/crypto_generichash/blake2b/ref/blake2b-compress-ssse3.c
    ${LIB_SODIUM_SRC_PATH}/crypto_generichash/blake2b/ref/blake2b-ref.c
    ${LIB_SODIUM_SRC_PATH}/crypto_generichash/blake2b/ref/generichash_blake2b.c
    ${LIB_SODIUM_SRC_PATH}/crypto_generichash/crypto_generichash.c
    ${LIB_SODIUM_SRC_PATH}/crypto_hash/crypto_hash.c
    ${LIB_SODIUM_SRC_PATH}/crypto_hash/sha256/cp/hash_sha256_cp.c
    ${LIB_SODIUM_SRC_PATH}/crypto_hash/sha256/hash_sha256.c
    ${LIB_SODIUM_SRC_PATH}/crypto_hash/sha512/cp/hash_sha512_cp.c
    ${LIB_SODIUM_SRC_PATH}/crypto_hash/sha512/hash_sha512.c
    ${LIB_SODIUM_SRC_PATH}/crypto_kdf/blake2b/kdf_blake2b.c
    ${LIB_SODIUM_SRC_PATH}/crypto_kdf/crypto_kdf.c
    ${LIB_SODIUM_SRC_PATH}/crypto_kx/crypto_kx.c
    ${LIB_SODIUM_SRC_PATH}/crypto_onetimeauth/crypto_onetimeauth.c
    ${LIB_SODIUM_SRC_PATH}/crypto_onetimeauth/poly1305/donna/poly1305_donna.c
    ${LIB_SODIUM_SRC_PATH}/crypto_onetimeauth/poly1305/onetimeauth_poly1305.c
    ${LIB_SODIUM_SRC_PATH}/crypto_onetimeauth/poly1305/sse2/poly1305_sse2.c
    ${LIB_SODIUM_SRC_PATH}/crypto_pwhash/argon2/argon2-core.c
    ${LIB_SODIUM_SRC_PATH}/crypto_pwhash/argon2/argon2-encoding.c
    ${LIB_SODIUM_SRC_PATH}/crypto_pwhash/argon2/argon2-fill-block-avx2.c
    ${LIB_SODIUM_SRC_PATH}/crypto_pwhash/argon2/argon2-fill-block-avx512f.c
    ${LIB_SODIUM_SRC_PATH}/crypto_pwhash/argon2/argon2-fill-block-ref.c
    ${LIB_SODIUM_SRC_PATH}/crypto_pwhash/argon2/argon2-fill-block-ssse3.c
    ${LIB_SODIUM_SRC_PATH}/crypto_pwhash/argon2/argon2.c
    ${LIB_SODIUM_SRC_PATH}/crypto_pwhash/argon2/blake2b-long.c
    ${LIB_SODIUM_SRC_PATH}/crypto_pwhash/argon2/pwhash_argon2i.c
    ${LIB_SODIUM_SRC_PATH}/crypto_pwhash/argon2/pwhash_argon2id.c
    ${LIB_SODIUM_SRC_PATH}/crypto_pwhash/crypto_pwhash.c
    ${LIB_SODIUM_SRC_PATH}/crypto_scalarmult/crypto_scalarmult.c
    ${LIB_SODIUM_SRC_PATH}/crypto_scalarmult/curve25519/ref10/x25519_ref10.c
    ${LIB_SODIUM_SRC_PATH}/crypto_scalarmult/curve25519/sandy2x/curve25519_sandy2x.c
    ${LIB_SODIUM_SRC_PATH}/crypto_scalarmult/curve25519/sandy2x/fe51_invert.c
    ${LIB_SODIUM_SRC_PATH}/crypto_scalarmult/curve25519/sandy2x/fe_frombytes_sandy2x.c
    ${LIB_SODIUM_SRC_PATH}/crypto_scalarmult/curve25519/scalarmult_curve25519.c
    ${LIB_SODIUM_SRC_PATH}/crypto_secretbox/crypto_secretbox.c
    ${LIB_SODIUM_SRC_PATH}/crypto_secretbox/crypto_secretbox_easy.c
    ${LIB_SODIUM_SRC_PATH}/crypto_secretbox/xsalsa20poly1305/secretbox_xsalsa20poly1305.c
    ${LIB_SODIUM_SRC_PATH}/crypto_secretstream/xchacha20poly1305/secretstream_xchacha20poly1305.c
    ${LIB_SODIUM_SRC_PATH}/crypto_shorthash/crypto_shorthash.c
    ${LIB_SODIUM_SRC_PATH}/crypto_shorthash/siphash24/ref/shorthash_siphash24_ref.c
    ${LIB_SODIUM_SRC_PATH}/crypto_shorthash/siphash24/shorthash_siphash24.c
    ${LIB_SODIUM_SRC_PATH}/crypto_sign/crypto_sign.c
    ${LIB_SODIUM_SRC_PATH}/crypto_sign/ed25519/ref10/keypair.c
    ${LIB_SODIUM_SRC_PATH}/crypto_sign/ed25519/ref10/open.c
    ${LIB_SODIUM_SRC_PATH}/crypto_sign/ed25519/ref10/sign.c
    ${LIB_SODIUM_SRC_PATH}/crypto_sign/ed25519/sign_ed25519.c
    ${LIB_SODIUM_SRC_PATH}/crypto_stream/chacha20/dolbeau/chacha20_dolbeau-avx2.c
    ${LIB_SODIUM_SRC_PATH}/crypto_stream/chacha20/dolbeau/chacha20_dolbeau-ssse3.c
    ${LIB_SODIUM_SRC_PATH}/crypto_stream/chacha20/ref/chacha20_ref.c
    ${LIB_SODIUM_SRC_PATH}/crypto_stream/chacha20/stream_chacha20.c
    ${LIB_SODIUM_SRC_PATH}/crypto_stream/crypto_stream.c
    ${LIB_SODIUM_SRC_PATH}/crypto_stream/salsa20/ref/salsa20_ref.c
    ${LIB_SODIUM_SRC_PATH}/crypto_stream/salsa20/stream_salsa20.c
    ${LIB_SODIUM_SRC_PATH}/crypto_stream/salsa20/xmm6/salsa20_xmm6.c
    ${LIB_SODIUM_SRC_PATH}/crypto_stream/salsa20/xmm6int/salsa20_xmm6int-avx2.c
    ${LIB_SODIUM_SRC_PATH}/crypto_stream/salsa20/xmm6int/salsa20_xmm6int-sse2.c
    ${LIB_SODIUM_SRC_PATH}/crypto_stream/xsalsa20/stream_xsalsa20.c
    ${LIB_SODIUM_SRC_PATH}/crypto_verify/verify.c
    ${LIB_SODIUM_SRC_PATH}/randombytes/internal/randombytes_internal_random.c
    ${LIB_SODIUM_SRC_PATH}/randombytes/randombytes.c
    ${LIB_SODIUM_SRC_PATH}/randombytes/sysrandom/randombytes_sysrandom.c
    ${LIB_SODIUM_SRC_PATH}/sodium/codecs.c
    ${LIB_SODIUM_SRC_PATH}/sodium/core.c
    ${LIB_SODIUM_SRC_PATH}/sodium/runtime.c
    ${LIB_SODIUM_SRC_PATH}/sodium/utils.c
)


if (POMELO_MULTI_THREAD)
    set(POMELO_COMPILE_DEFINES POMELO_MULTI_THREAD=1)
endif()


if(MSVC)
    set(POMELO_COMPILE_FLAGS /WX /Wv:18)
    set(POMELO_LINK_FLAGS)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
    set(POMELO_COMPILE_FLAGS -Wall -Wpedantic -Wextra -Werror)
    set(POMELO_LINK_FLAGS -ldl)
endif()


# Sodium
add_library(sodium STATIC ${SRC_SODIUM})
target_include_directories(sodium PUBLIC ${INCLUDE_SODIUM})
target_compile_definitions(sodium PUBLIC SODIUM_STATIC=1 DEV_MODE=0 CONFIGURED=1)


# Base
add_library(${POMELO_BASE} STATIC ${SRC_BASE})
target_include_directories(${POMELO_BASE} PRIVATE ${INCLUDE_UV} ${INCLUDE_POMELO})
target_compile_definitions(${POMELO_BASE} PRIVATE ${POMELO_COMPILE_DEFINES})
target_compile_options(${POMELO_BASE} PRIVATE ${POMELO_COMPILE_FLAGS})
target_link_libraries(${POMELO_BASE} PRIVATE uv_a)


# Protocol
add_library(${POMELO_PROTOCOL} STATIC ${SRC_PROTOCOL})
target_include_directories(${POMELO_PROTOCOL} PRIVATE ${INCLUDE_POMELO})
target_compile_definitions(${POMELO_PROTOCOL} PRIVATE ${POMELO_COMPILE_DEFINES})
target_compile_options(${POMELO_PROTOCOL} PRIVATE ${POMELO_COMPILE_FLAGS})


# Adapter default
add_library(${POMELO_ADAPTER_DEFAULT} STATIC ${SRC_ADAPTER_BASE} ${SRC_ADAPTER_DEFAULT})
target_include_directories(${POMELO_ADAPTER_DEFAULT} PRIVATE ${INCLUDE_POMELO})
target_compile_definitions(${POMELO_ADAPTER_DEFAULT} PRIVATE ${POMELO_COMPILE_DEFINES})
target_compile_options(${POMELO_ADAPTER_DEFAULT} PRIVATE ${POMELO_COMPILE_FLAGS})


# Codec
add_library(${POMELO_CODEC} STATIC ${SRC_CODEC})
target_compile_definitions(${POMELO_CODEC} PRIVATE ${POMELO_COMPILE_DEFINES})
target_include_directories(${POMELO_CODEC} PRIVATE ${INCLUDE_POMELO} ${INCLUDE_SODIUM})
target_link_libraries(${POMELO_CODEC} PRIVATE sodium)
target_compile_options(${POMELO_CODEC} PRIVATE ${POMELO_COMPILE_FLAGS})


# Utils
add_library(${POMELO_UTILS} STATIC ${SRC_UTILS})
target_include_directories(${POMELO_UTILS} PRIVATE ${INCLUDE_UV} ${INCLUDE_POMELO})
target_compile_definitions(${POMELO_UTILS} PRIVATE ${POMELO_COMPILE_DEFINES})
target_compile_options(${POMELO_UTILS} PRIVATE ${POMELO_COMPILE_FLAGS})


# Delivery
add_library(${POMELO_DELIVERY} STATIC ${SRC_DELIVERY})
target_include_directories(${POMELO_DELIVERY} PRIVATE ${INCLUDE_POMELO})
target_compile_definitions(${POMELO_DELIVERY} PRIVATE ${POMELO_COMPILE_DEFINES})
target_compile_options(${POMELO_DELIVERY} PRIVATE ${POMELO_COMPILE_FLAGS})


# Platform UV
add_library(${POMELO_PLATFORM_UV} STATIC ${SRC_PLATFORM_UV})
target_include_directories(${POMELO_PLATFORM_UV} PUBLIC ${INCLUDE_UV} PRIVATE ${INCLUDE_POMELO})
target_compile_definitions(${POMELO_PLATFORM_UV} PRIVATE ${POMELO_COMPILE_DEFINES})
target_compile_options(${POMELO_PLATFORM_UV} PRIVATE ${POMELO_COMPILE_FLAGS})
target_link_libraries(${POMELO_PLATFORM_UV} PRIVATE uv_a)


# Platform Poll
add_library(${POMELO_PLATFORM_POLL} STATIC ${SRC_PLATFORM_POLL})
target_include_directories(${POMELO_PLATFORM_POLL} PRIVATE ${INCLUDE_UV} ${INCLUDE_POMELO})
target_compile_definitions(${POMELO_PLATFORM_POLL} PRIVATE ${POMELO_COMPILE_DEFINES})
target_compile_options(${POMELO_PLATFORM_POLL} PRIVATE ${POMELO_COMPILE_FLAGS})
target_link_libraries(${POMELO_PLATFORM_POLL} PRIVATE uv_a)


# API
add_library(${POMELO_API} STATIC ${SRC_API})
target_include_directories(${POMELO_API} PUBLIC ${INCLUDE_POMELO})
target_compile_definitions(${POMELO_API} PRIVATE ${POMELO_COMPILE_DEFINES})
target_compile_options(${POMELO_API} PRIVATE ${POMELO_COMPILE_FLAGS})


# Generator
add_executable(${POMELO_GENERATOR} ${SRC_GENERATOR})
target_include_directories(${POMELO_GENERATOR} PRIVATE ${INCLUDE_POMELO} ${INCLUDE_SODIUM})
target_compile_definitions(${POMELO_GENERATOR} PRIVATE ${POMELO_COMPILE_DEFINES})
target_link_libraries(${POMELO_GENERATOR} PRIVATE
    ${POMELO_BASE}
    ${POMELO_CODEC}
    ${POMELO_UTILS}
    uv_a
    sodium
)
target_compile_options(${POMELO_GENERATOR} PRIVATE ${POMELO_COMPILE_FLAGS})


# Test base
add_executable(${POMELO_TEST_BASE} ${SRC_TEST_BASE})
target_include_directories(${POMELO_TEST_BASE} PRIVATE ${INCLUDE_POMELO_TEST})
target_link_libraries(${POMELO_TEST_BASE} PRIVATE
    ${POMELO_BASE}
    ${POMELO_UTILS}
    ${POMELO_PLATFORM_UV}
    ${POMELO_CODEC}
)
target_compile_options(${POMELO_TEST_BASE} PRIVATE ${POMELO_COMPILE_FLAGS})


# Test Platform UV
add_executable(${POMELO_TEST_PLATFORM_UV} ${SRC_TEST_PLATFORM_UV})
target_include_directories(${POMELO_TEST_PLATFORM_UV} PRIVATE ${INCLUDE_POMELO_TEST})
target_link_libraries(${POMELO_TEST_PLATFORM_UV} PRIVATE ${POMELO_UTILS} ${POMELO_BASE} ${POMELO_PLATFORM_UV})
target_compile_options(${POMELO_TEST_PLATFORM_UV} PRIVATE ${POMELO_COMPILE_FLAGS})


# Test Platform Poll
add_executable(${POMELO_TEST_PLATFORM_POLL} ${SRC_TEST_PLATFORM_POLL})
target_include_directories(${POMELO_TEST_PLATFORM_POLL} PRIVATE ${INCLUDE_POMELO_TEST} ${INCLUDE_UV})
target_link_libraries(${POMELO_TEST_PLATFORM_POLL} PRIVATE ${POMELO_UTILS} ${POMELO_BASE} ${POMELO_PLATFORM_POLL})
target_compile_options(${POMELO_TEST_PLATFORM_POLL} PRIVATE ${POMELO_COMPILE_FLAGS})


# Test protocol: Unencrypted packets tranfering
add_executable(${POMELO_TEST_PROTOCOL_UNENCRYPTED} ${SRC_TEST_PROTOCOL_UNENCRYPTED})
target_include_directories(${POMELO_TEST_PROTOCOL_UNENCRYPTED} PRIVATE ${INCLUDE_POMELO_TEST})
target_link_libraries(${POMELO_TEST_PROTOCOL_UNENCRYPTED} PRIVATE
    ${POMELO_BASE}
    ${POMELO_PROTOCOL}
    ${POMELO_UTILS}
    ${POMELO_PLATFORM_UV}
    ${POMELO_CODEC}
)
target_compile_options(${POMELO_TEST_PROTOCOL_UNENCRYPTED} PRIVATE ${POMELO_COMPILE_FLAGS})


# Test protocol: Client
add_executable(${POMELO_TEST_PROTOCOL_CLIENT} ${SRC_TEST_PROTOCOL_CLIENT})
target_include_directories(${POMELO_TEST_PROTOCOL_CLIENT} PRIVATE ${INCLUDE_POMELO_TEST})
target_link_libraries(${POMELO_TEST_PROTOCOL_CLIENT} PRIVATE
    ${POMELO_BASE}
    ${POMELO_PROTOCOL}
    ${POMELO_UTILS}
    ${POMELO_PLATFORM_UV}
    ${POMELO_CODEC}
)
target_compile_options(${POMELO_TEST_PROTOCOL_CLIENT} PRIVATE ${POMELO_COMPILE_FLAGS})
target_compile_definitions(${POMELO_TEST_PROTOCOL_CLIENT} PRIVATE)


# Test protocol: Server
add_executable(${POMELO_TEST_PROTOCOL_SERVER} ${SRC_TEST_PROTOCOL_SERVER})
target_include_directories(${POMELO_TEST_PROTOCOL_SERVER} PRIVATE ${INCLUDE_POMELO_TEST})
target_link_libraries(${POMELO_TEST_PROTOCOL_SERVER} PRIVATE
    ${POMELO_BASE}
    ${POMELO_PROTOCOL}
    ${POMELO_UTILS}
    ${POMELO_PLATFORM_UV}
    ${POMELO_CODEC}
)
target_compile_options(${POMELO_TEST_PROTOCOL_SERVER} PRIVATE ${POMELO_COMPILE_FLAGS})
target_compile_definitions(${POMELO_TEST_PROTOCOL_SERVER} PRIVATE)


add_executable(${POMELO_TEST_PROTOCOL} ${SRC_TEST_PROTOCOL})
target_include_directories(${POMELO_TEST_PROTOCOL} PRIVATE ${INCLUDE_POMELO_TEST})
target_link_libraries(${POMELO_TEST_PROTOCOL} PRIVATE
    ${POMELO_BASE}
    ${POMELO_PROTOCOL}
    ${POMELO_UTILS}
    ${POMELO_PLATFORM_UV}
    ${POMELO_CODEC}
    ${POMELO_ADAPTER_DEFAULT}
)
target_compile_options(${POMELO_TEST_PROTOCOL} PRIVATE ${POMELO_COMPILE_FLAGS})


add_executable(${POMELO_TEST_DELIVERY} ${SRC_TEST_DELIVERY})
target_include_directories(${POMELO_TEST_DELIVERY} PRIVATE ${INCLUDE_POMELO_TEST})
target_link_libraries(${POMELO_TEST_DELIVERY} PRIVATE
    ${POMELO_BASE}
    ${POMELO_UTILS}
    ${POMELO_CODEC}
    ${POMELO_DELIVERY}
    ${POMELO_PLATFORM_UV}
    uv_a
)
target_compile_options(${POMELO_TEST_DELIVERY} PRIVATE ${POMELO_COMPILE_FLAGS})


# Test API with UV platform
add_executable(${POMELO_TEST_API_UV} ${SRC_TEST_API_UV})
target_include_directories(${POMELO_TEST_API_UV} PRIVATE ${INCLUDE_POMELO_TEST})
target_link_libraries(${POMELO_TEST_API_UV} PRIVATE
    ${POMELO_BASE}
    ${POMELO_PROTOCOL}
    ${POMELO_UTILS}
    ${POMELO_PLATFORM_UV}
    ${POMELO_CODEC}
    ${POMELO_DELIVERY}
    ${POMELO_API}
    ${POMELO_ADAPTER_DEFAULT}
)
target_compile_options(${POMELO_TEST_API_UV} PRIVATE ${POMELO_COMPILE_FLAGS})


# Test API with Poll platform
add_executable(${POMELO_TEST_API_POLL} ${SRC_TEST_API_POLL})
target_include_directories(${POMELO_TEST_API_POLL} PRIVATE ${INCLUDE_POMELO_TEST} ${INCLUDE_UV})
target_link_libraries(${POMELO_TEST_API_POLL} PRIVATE
    ${POMELO_BASE}
    ${POMELO_PROTOCOL}
    ${POMELO_UTILS}
    ${POMELO_PLATFORM_POLL}
    ${POMELO_CODEC}
    ${POMELO_DELIVERY}
    ${POMELO_API}
    ${POMELO_ADAPTER_DEFAULT}
)
target_compile_options(${POMELO_TEST_API_POLL} PRIVATE ${POMELO_COMPILE_FLAGS})


# Demo plugin
add_library(${POMELO_TEST_DEMO_PLUGIN} SHARED ${SRC_TEST_DEMO_PLUGIN})
target_include_directories(${POMELO_TEST_DEMO_PLUGIN} PRIVATE include)


# Test demo plugin
add_executable(${POMELO_TEST_PLUGIN} ${SRC_TEST_PLUGIN})
target_include_directories(${POMELO_TEST_PLUGIN} PRIVATE ${INCLUDE_POMELO_TEST})
target_link_libraries(${POMELO_TEST_PLUGIN} PRIVATE
    ${POMELO_BASE}
    ${POMELO_PROTOCOL}
    ${POMELO_UTILS}
    ${POMELO_PLATFORM_UV}
    ${POMELO_CODEC}
    ${POMELO_DELIVERY}
    ${POMELO_API}
    ${POMELO_ADAPTER_DEFAULT}
)
target_link_options(${POMELO_TEST_PLUGIN} PRIVATE ${POMELO_LINK_FLAGS})
add_dependencies(${POMELO_TEST_PLUGIN} ${POMELO_TEST_DEMO_PLUGIN})
target_compile_options(${POMELO_TEST_PLUGIN} PRIVATE ${POMELO_COMPILE_FLAGS})


# Test WebRTC plugin
add_executable(${POMELO_TEST_WEBRTC_PLUGIN} ${SRC_TEST_WEBRTC_PLUGIN})
target_include_directories(${POMELO_TEST_WEBRTC_PLUGIN} PUBLIC ${INCLUDE_POMELO_TEST})
target_link_libraries(${POMELO_TEST_WEBRTC_PLUGIN} PRIVATE 
    ${POMELO_BASE}
    ${POMELO_PROTOCOL}
    ${POMELO_UTILS}
    ${POMELO_PLATFORM_UV}
    ${POMELO_CODEC}
    ${POMELO_DELIVERY}
    ${POMELO_API}
    ${POMELO_ADAPTER_DEFAULT}
)
target_link_options(${POMELO_TEST_WEBRTC_PLUGIN} PRIVATE ${POMELO_LINK_FLAGS})
target_compile_options(${POMELO_TEST_WEBRTC_PLUGIN} PRIVATE ${POMELO_COMPILE_FLAGS})


add_library(${POMELO_EXAMPLE_SHARED} example/shared.h example/shared.c)
target_link_libraries(${POMELO_EXAMPLE_SHARED} PUBLIC
    ${POMELO_BASE}
    ${POMELO_PROTOCOL}
    ${POMELO_UTILS}
    ${POMELO_PLATFORM_UV}
    ${POMELO_CODEC}
    ${POMELO_DELIVERY}
    ${POMELO_API}
    ${POMELO_ADAPTER_DEFAULT}
)
target_link_options(${POMELO_EXAMPLE_SHARED} PRIVATE ${POMELO_LINK_FLAGS})
target_compile_options(${POMELO_EXAMPLE_SHARED} PRIVATE ${POMELO_COMPILE_FLAGS})


# Example client
add_executable(${POMELO_EXAMPLE_CLIENT} example/client.c)
target_link_libraries(${POMELO_EXAMPLE_CLIENT} PRIVATE ${POMELO_EXAMPLE_SHARED})
target_link_options(${POMELO_EXAMPLE_CLIENT} PRIVATE ${POMELO_LINK_FLAGS})
target_compile_options(${POMELO_EXAMPLE_CLIENT} PRIVATE ${POMELO_COMPILE_FLAGS})


# Example server
add_executable(${POMELO_EXAMPLE_SERVER} example/server.c)
target_link_libraries(${POMELO_EXAMPLE_SERVER} PRIVATE ${POMELO_EXAMPLE_SHARED})
target_link_options(${POMELO_EXAMPLE_SERVER} PRIVATE ${POMELO_LINK_FLAGS})
target_compile_options(${POMELO_EXAMPLE_SERVER} PRIVATE ${POMELO_COMPILE_FLAGS})
