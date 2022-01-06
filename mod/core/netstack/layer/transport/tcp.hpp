#pragma once

#include "aex/net/ipv4.hpp"
#include "aex/utility.hpp"

#include <stdint.h>

using namespace AEX;

namespace NetStack {
    struct tcp_header {
        be<uint16_t> source_port;
        be<uint16_t> destination_port;

        be<uint32_t> seq_number;
        be<uint32_t> ack_number;

        be<uint16_t> goddamned_bitvalues;
        be<uint16_t> window;

        be<uint16_t> checksum;
        be<uint16_t> urgent_pointer;

        uint8_t options[];
    } PACKED;

    static_assert(sizeof(tcp_header) == 20);

    struct tcp_fake_ipv4_header {
        Net::ipv4_addr source;
        Net::ipv4_addr destination;
        uint8_t        zero;
        uint8_t        protocol;
        be<uint16_t>   length;
    } PACKED;

    class TCPLayer {
        public:
        static void init();

        static void parse(const uint8_t* buffer, uint16_t len, Net::ipv4_addr src_addr,
                          Net::ipv4_addr dst_addr);
    };
}