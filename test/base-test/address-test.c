#include "pomelo-test.h"
#include "base-test.h"
#include "pomelo/address.h"
#ifdef _WIN32
#include <winsock.h>
#else
#include <arpa/inet.h>
#endif

int pomelo_test_address(void) {
    pomelo_address_t address;
    int ret;

    // IPv4
    pomelo_check(pomelo_address_from_string(&address, "4.1.200.34:1234") == 0);
    pomelo_check(address.port == htons(1234));
    pomelo_check(address.type == POMELO_ADDRESS_IPV4);

    pomelo_check(address.ip.v4[0] == 4);
    pomelo_check(address.ip.v4[1] == 1);
    pomelo_check(address.ip.v4[2] == 200);
    pomelo_check(address.ip.v4[3] == 34);

    pomelo_check(pomelo_address_from_string(&address, "5.6.44.34") != 0);
    pomelo_check(pomelo_address_from_string(&address, "5.6.44.34.213") != 0);
    pomelo_check(pomelo_address_from_string(&address, "5.6....") != 0);

    // IPv6
    ret = pomelo_address_from_string(
        &address, "[fe80::ce81:b1c:bd2c:69e]:4322"
    );
    pomelo_check(ret == 0);

    pomelo_check(address.type == POMELO_ADDRESS_IPV6);
    pomelo_check(address.port == htons(4322));
    pomelo_check(address.ip.v6[0] == htons(0xfe80));
    pomelo_check(address.ip.v6[1] == htons(0x0000));
    pomelo_check(address.ip.v6[2] == htons(0x0000));
    pomelo_check(address.ip.v6[3] == htons(0x0000));
    pomelo_check(address.ip.v6[4] == htons(0xce81));
    pomelo_check(address.ip.v6[5] == htons(0x0b1c));
    pomelo_check(address.ip.v6[6] == htons(0xbd2c));
    pomelo_check(address.ip.v6[7] == htons(0x069e));

    ret = pomelo_address_from_string(&address, "fe80::ce81:b1c:bd2c::4322");
    pomelo_check(ret != 0);

    ret = pomelo_address_from_string(&address, "fe80::ce81:b1c:bd2c:69e");
    pomelo_check(ret != 0);

    return 0;
}
