#pragma once

namespace AEX {
    template <typename T>
    inline T max(T a, T b) {
        return (a > b) ? a : b;
    }

    template <typename T>
    inline T min(T a, T b) {
        return (a < b) ? a : b;
    }

    template <typename T>
    inline T int_floor(T val, T alignment) {
        return (val / alignment) * alignment;
    }

    template <typename T>
    inline T int_ceil(T val, T alignment) {
        return ((val + alignment - 1) / alignment) * alignment;
    }
} // namespace AEX