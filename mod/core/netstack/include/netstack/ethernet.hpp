#pragma once

#include "aex/byte.hpp"         // meh
#include "aex/net/ethernet.hpp" // meh
#include "aex/utility.hpp"

#include <stdint.h>

namespace NetStack {
    enum ethertype_t : uint16_t {
        ETH_IPv4 = 0x0800,
        ETH_ARP  = 0x0806,
        ETH_IPv6 = 0x08DD,
    };

    struct ethernet_frame {
        Net::mac_addr destination;
        Net::mac_addr source;
        be<uint16_t>  ethertype;

        uint8_t payload[];
    } PACKED;
}