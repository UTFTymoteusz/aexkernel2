#pragma once

#include "aex/utility.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX {
    API inline int16_t bswap(int16_t x) {
        return ((x & 0xFF00) >> 8) | (x << 8);
    }

    API inline uint16_t bswap(uint16_t x) {
        return ((x & 0xFF00) >> 8) | (x << 8);
    }

    API inline int32_t bswap(int32_t x) {
        return ((x & 0xFF000000) >> 24) | ((x & 0x00FF0000) >> 8) | ((x & 0x0000FF00) << 8) |
               (x << 24);
    }

    API inline uint32_t bswap(uint32_t x) {
        return ((x & 0xFF000000) >> 24) | ((x & 0x00FF0000) >> 8) | ((x & 0x0000FF00) << 8) |
               (x << 24);
    }

    template <typename T>
    API inline T from_le(T x) {
        return BIG_ENDIAN ? bswap(x) : x;
    }

    template <typename T>
    API inline T to_le(T x) {
        return BIG_ENDIAN ? bswap(x) : x;
    }

    template <typename T>
    API inline T from_be(T x) {
        return LITTLE_ENDIAN ? bswap(x) : x;
    }

    template <typename T>
    API inline T to_be(T x) {
        return LITTLE_ENDIAN ? bswap(x) : x;
    }

    template <typename T>
    struct API le {
        T m_value;

        inline T get() {
            return from_le<T>(m_value);
        }

        inline void set(T value) {
            m_value = to_le<T>(value);
        }

        operator T() {
            return get();
        }

        le& operator=(const T& value) {
            set(value);
            return *this;
        }
    } PACKED;

    template <typename T>
    struct API be {
        T m_value;

        inline T get() {
            return from_be<T>(m_value);
        }

        inline void set(T value) {
            m_value = to_be<T>(value);
        }

        operator T() {
            return get();
        }

        be& operator=(const T& value) {
            set(value);
            return *this;
        }
    } PACKED;

    API inline int8_t fromBCD(int8_t val) {
        return ((val & 0xF0) >> 1) + ((val & 0xF0) >> 3) + (val & 0x0F);
    }
}