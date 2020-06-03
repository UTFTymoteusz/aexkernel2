#pragma once

#include "aex/string.hpp"

#include <stdint.h>

namespace AEX::Net {
    struct ipv4_addr {
        uint8_t bytes[4];

        ipv4_addr(const uint8_t ipv4[6]) {
            memcpy(bytes, ipv4, 4);
        }

        uint8_t& operator[](int index) {
            return bytes[index];
        }

        bool operator==(const ipv4_addr& b) {
            return memcmp(bytes, b.bytes, 6) == 0;
        }

        bool operator!=(const ipv4_addr& b) {
            return memcmp(bytes, b.bytes, 6) != 0;
        }
    } __attribute__((packed));
}