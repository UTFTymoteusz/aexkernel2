#pragma once

#include "aex/byte.hpp"
#include "aex/dev.hpp"
#include "aex/net.hpp"
#include "aex/utility.hpp"

#include <stdint.h>

using namespace AEX;

namespace NetStack {
    struct udp_header {
        be<uint16_t> source_port;
        be<uint16_t> destination_port;
        be<uint16_t> total_length;
        be<uint16_t> checksum;
    } PACKED;

    struct udp_fake_ipv4_header {
        Net::ipv4_addr source;
        Net::ipv4_addr destination;
        uint8_t        zero;
        uint8_t        protocol;
        be<uint16_t>   length;
    } PACKED;

    class UDPLayer {
        public:
        static void parse(const uint8_t* buffer, uint16_t len, Net::ipv4_addr src_addr,
                          Net::ipv4_addr dst_addr);
    };
}
