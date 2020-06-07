#pragma once

#include "aex/errno.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX::Net {
    enum link_type_t : uint8_t {
        LINK_NONE     = 0,
        LINK_ETHERNET = 1,
    };

    class LinkLayer;

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
}