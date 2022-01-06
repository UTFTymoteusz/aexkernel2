#pragma once

#include "aex/byte.hpp"         // meh
#include "aex/net/ethernet.hpp" // meh
#include "aex/net/ipv4.hpp"     // meh
#include "aex/optional.hpp"

#include <stdint.h>

using namespace AEX;

namespace NetStack {
    enum arp_hardware {
        ARP_ETHERNET        = 1,
        ARP_XETHERNET       = 2,
        ARP_AX25            = 3,
        ARP_TOKENRING       = 4,
        ARP_CHAOS           = 5,
        ARP_IEEE802         = 6,
        ARP_ARCNET          = 7,
        ARP_HYPERCHANNEL    = 8,
        ARP_LANSTAR         = 9,
        ARP_AUTONET         = 10,
        ARP_LOCALTALK       = 11,
        ARP_LOCALNET        = 12,
        ARP_ULTRALINK       = 13,
        ARP_SMDS            = 14,
        ARP_FRAMERELAY      = 15,
        ARP_ATM             = 16,
        ARP_HDLC            = 17,
        ARP_FIBRECHANNEL    = 18,
        ARP_ATM1            = 19,
        ARP_SERIALLINE      = 20,
        ARP_ATM2            = 21,
        ARP_MIL_STD_188_220 = 22,
        ARP_METRICOM        = 23,
        ARP_IEEE1394        = 24,
        ARP_MAPOS           = 25,
        ARP_TWINAXIAL       = 26,
        ARP_EUI_64          = 27,
        ARP_HIPARP          = 28,
        ARP_ISO7816_3       = 29,
        ARP_ARPSEC          = 30,
        ARP_IPSEC           = 31,
        ARP_INFINIBAND      = 32,
        ARP_TIA_102         = 33,
        ARP_WIEGAND         = 34,
        ARP_PUREIP          = 35,
        ARP_HW_EXP1         = 36,
        ARP_HFI             = 37,
        ARP_HW_EXP2         = 256,
        ARP_AETHERNET       = 257,
    };

    enum arp_operation {
        ARP_REQUEST = 1,
        ARP_REPLY   = 2,
    };

    struct arp_header {
        be<uint16_t> hardware_type;
        be<uint16_t> protocol_type;

        uint8_t hardware_len;
        uint8_t protocol_len;

        be<uint16_t> operation;
    } PACKED;

    struct arp_eth_ipv4 {
        Net::mac_addr  source_mac;
        Net::ipv4_addr source_ipv4;
        Net::mac_addr  destination_mac;
        Net::ipv4_addr destination_ipv4;

        char footer[12];
    } PACKED;

    struct arp_raw {
        uint8_t raw[4];
    } PACKED;

    struct arp_packet {
        arp_header header;

        union {
            arp_eth_ipv4 eth_ipv4;
            arp_raw      raw;
        } PACKED;

        arp_packet() {}
    } PACKED;

    optional<Net::mac_addr> arp_get_mac(Net::ipv4_addr ipv4);
}