#pragma once

namespace AEX {
    template <typename T, typename U>
    constexpr bool equals_one(T a, U b) {
        return a == b;
    }

    template <typename T, typename U, typename... V>
    constexpr bool equals_one(T a, U b, V... c) {
        if (equals_one(a, b) || equals_one(a, c...))
            return true;

        return false;
    }
}