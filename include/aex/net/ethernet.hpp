#pragma once

#include "aex/string.hpp"

#include <stdint.h>

namespace AEX::Net {
    struct mac_addr {
        uint8_t bytes[6];

        mac_addr(const uint8_t mac[6]) {
            memcpy(bytes, mac, 6);
        }

        bool isBroadcast() {
            return bytes[0] & 0x01;
        }

        uint8_t& operator[](int index) {
            return bytes[index];
        }

        bool operator==(const mac_addr& b) {
            return memcmp(bytes, b.bytes, 6) == 0;
        }

        bool operator!=(const mac_addr& b) {
            return memcmp(bytes, b.bytes, 6) != 0;
        }
    } __attribute__((packed));
}