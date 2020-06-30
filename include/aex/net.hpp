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

    class LinkLayer;
    class INetProtocol;

    struct ipv4_addr;

    extern INetProtocol** inet_protocols;

    /**
     * Registers a link layer in the network subsystem.
     * @param type Link layer type.
     * @param layer Pointer to the layer class.
     * @returns Preferably ENONE, an error otherwise.
     */
    error_t register_link_layer(link_type_t type, LinkLayer* layer);

    /**
     * Parses a packet.
     * @param device_id Recipient device ID.
     * @param type Layer type to start with.
     * @param packet Pointer to the packet.
     * @param len Packet length.
     */
    void parse(int device_id, link_type_t type, const void* packet, size_t len);

    /**
     * Registers an inet protocol in the network subsystem.
     * @param id Protocol ID.
     * @param protocol Pointer to the protocol class.
     * @returns Preferably ENONE, an error otherwise.
     */
    error_t register_inet_protocol(socket_protocol_t id, INetProtocol* protocol);
}