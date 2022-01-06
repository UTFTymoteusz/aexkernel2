#pragma once

#include "aex/errno.hpp"
#include "aex/net/domain.hpp"
#include "aex/net/ipv4.hpp"
#include "aex/net/protocol.hpp"
#include "aex/net/socket.hpp"
#include "aex/optional.hpp"
#include "aex/utility.hpp"

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

    class Domain;
    extern API Domain** domains;

    /**
     * Registers a domain in the network subsystem.
     * @param id Domain ID.
     * @param protocol Pointer to the domain class.
     * @returns Preferably ENONE, an error otherwise.
     **/
    API error_t register_domain(domain_t af, Domain* protocol);

    API optional<char*> get_hostname(char* buffer, size_t len);
    API void            set_hostname(const char* hostname);
}