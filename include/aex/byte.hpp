#pragma once

#include "aex/utility.hpp"

#include <stddef.h>
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
    inline T from_little_endian(T x) {
        return BIG_ENDIAN ? bswap(x) : x;
    }

    template <typename T>
    inline T to_little_endian(T x) {
        return BIG_ENDIAN ? bswap(x) : x;
    }

    template <typename T>
    inline T from_big_endian(T x) {
        return LITTLE_ENDIAN ? bswap(x) : x;
    }

    template <typename T>
    inline T to_big_endian(T x) {
        return LITTLE_ENDIAN ? bswap(x) : x;
    }

    template <typename T>
    struct little_endian {
        T m_value;

        T get() {
            return from_little_endian<T>(m_value);
        }

        void set(T value) {
            m_value = to_little_endian<T>(value);
        }

        operator T() {
            return get();
        }

        little_endian& operator=(const T& value) {
            set(value);
            return *this;
        }
    } __attribute__((packed));

    template <typename T>
    struct big_endian {
        T m_value;

        T get() {
            return from_big_endian<T>(m_value);
        }

        void set(T value) {
            m_value = to_big_endian<T>(value);
        }

        operator T() {
            return get();
        }

        big_endian& operator=(const T& value) {
            set(value);
            return *this;
        }
    } __attribute__((packed));

    inline int8_t fromBCD(int8_t val) {
        return ((val & 0xF0) >> 1) + ((val & 0xF0) >> 3) + (val & 0x0F);
    }
}