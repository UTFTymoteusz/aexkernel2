#pragma once

#include "aex/byte.hpp"
#include "aex/net/ipv4.hpp"
#include "aex/utility.hpp"

#include <stdint.h>

using namespace AEX;

namespace NetStack {
    enum ipv4_protocol_t {
        IPv4_IPv4 = 4,
        IPv4_TCP  = 6,
        IPv4_UDP  = 17,
    };

    struct ipv4_header {
        uint8_t header_len : 4;
        uint8_t version : 4;

        uint8_t dsf;

        be<uint16_t> total_len;
        be<uint16_t> id;

        be<uint16_t> flags;

        uint8_t ttl;
        uint8_t protocol;

        be<uint16_t> header_checksum;

        Net::ipv4_addr source;
        Net::ipv4_addr destination;
    } PACKED;

    static constexpr Net::ipv4_addr IPv4_ANY       = Net::ipv4_addr(0, 0, 0, 0);
    static constexpr Net::ipv4_addr IPv4_BROADCAST = Net::ipv4_addr(255, 255, 255, 255);
}