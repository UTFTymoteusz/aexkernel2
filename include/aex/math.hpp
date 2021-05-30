#pragma once

#include "aex/utility.hpp"

namespace AEX {
    template <typename T>
    API constexpr T max(T a, T b) {
        return (a > b) ? a : b;
    }

    template <typename T, typename... Ts>
    API constexpr T max(T a, T b, Ts... c) {
        return max(a, max(b, c...));
    }

    template <typename T>
    API constexpr T min(T a, T b) {
        return (a < b) ? a : b;
    }

    template <typename T, typename... Ts>
    API constexpr T min(T a, T b, Ts... c) {
        return min(a, min(b, c...));
    }

    template <typename T>
    API constexpr T clamp(T a, T minv, T maxv) {
        return max(min(a, maxv), minv);
    }

    template <typename T, typename U>
    API constexpr T int_floor(T val, U alignment) {
        return (val / alignment) * alignment;
    }

    template <typename T, typename U>
    API constexpr T int_ceil(T val, U alignment) {
        return ((val + alignment - 1) / alignment) * alignment;
    }

    template <typename T, typename A, typename B>
    API constexpr bool inrange(T val, A min, B max) {
        return val >= min && val <= max;
    }
}