#pragma once

#include "aex/errno.hpp"
#include "aex/net/ipv4.hpp"
#include "aex/net/socket.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX::Net {
    enum link_type_t : uint8_t {
        LINK_NONE     = 0,
        LINK_ETHERNET = 1,
    };

    enum net_type_t : uint16_t {
        NET_IPv4 = 0x00,
        NET_IPv6 = 0x01,
        NET_ARP  = 0x02,
        NET_RAW  = 0xFFFF,
    };

    class INetProtocol;

    extern INetProtocol** inet_protocols;

    /**
     * Registers an inet protocol in the network subsystem.
     * @param id Protocol ID.
     * @param protocol Pointer to the protocol class.
     * @returns Preferably ENONE, an error otherwise.
     */
    error_t register_inet_protocol(socket_protocol_t id, INetProtocol* protocol);
}