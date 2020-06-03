#pragma once

#include <stdint.h>

namespace AEX {
    inline int16_t bswap(int16_t x) {
        return ((x & 0xFF00) >> 8) | (x << 8);
    }

    inline uint16_t bswap(uint16_t x) {
        return ((x & 0xFF00) >> 8) | (x << 8);
    }

    inline int32_t bswap(int32_t x) {
        return ((x & 0xFF000000) >> 24) | ((x & 0x00FF0000) >> 8) | ((x & 0x0000FF00) << 8) |
               (x << 24);
    }

    inline uint32_t bswap(uint32_t x) {
        return ((x & 0xFF000000) >> 24) | ((x & 0x00FF0000) >> 8) | ((x & 0x0000FF00) << 8) |
               (x << 24);
    }

    template <typename T>
    inline T fromBigEndian(T x) {
        return bswap(x);
    }

    template <typename T>
    inline T toBigEndian(T x) {
        return bswap(x);
    }
}