#pragma once

namespace AEX {
    template <typename T>
    constexpr T max(T a, T b) {
        return (a > b) ? a : b;
    }

    template <typename T, typename... Ts>
    constexpr T max(T a, T b, Ts... c) {
        return max(a, max(b, c...));
    }

    template <typename T>
    constexpr T min(T a, T b) {
        return (a < b) ? a : b;
    }

    template <typename T, typename... Ts>
    constexpr T min(T a, T b, Ts... c) {
        return min(a, min(b, c...));
    }

    template <typename T>
    constexpr T clamp(T a, T minv, T maxv) {
        return max(min(a, maxv), minv);
    }

    template <typename T>
    constexpr T int_floor(T val, T alignment) {
        return (val / alignment) * alignment;
    }

    template <typename T>
    constexpr T int_ceil(T val, T alignment) {
        return ((val + alignment - 1) / alignment) * alignment;
    }
}