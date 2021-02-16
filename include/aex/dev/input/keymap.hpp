#pragma once

#include "aex/utility.hpp"

#include <stdint.h>

namespace AEX::Dev::Input {
    struct API key {
        char normal;
        char shift;
        char ctrl;
        char ctrl_alt;

        char capslock;
        char capslock_shift;
        char capslock_ctrl;
        char capslock_ctrl_alt;

        static constexpr key nullKey() {
            return {'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0'};
        }

        static constexpr key normalKey(char lower, char upper) {
            return {lower, upper, '\0', '\0', upper, lower, '\0', '\0'};
        }

        static constexpr key symbolKey(char c) {
            return {c, c, '\0', '\0', c, c, '\0', '\0'};
        }

        static constexpr key symbolKey(char non_shift, char shift) {
            return {non_shift, shift, '\0', '\0', non_shift, shift, '\0', '\0'};
        }
    };

    struct API keymap {
        key keys[256];
    };
}