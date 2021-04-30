#pragma once

#include "aex/dev/input/code.hpp"
#include "aex/utility.hpp"

#include <stdint.h>

namespace AEX::Dev::Input {
    struct API event {
        hid_keycode_t keycode;
        keymod_t      mod;
    };

    static_assert(sizeof(event) == 2);
}