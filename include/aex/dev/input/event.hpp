#pragma once

#include <stdint.h>

namespace AEX::Dev::Input {
    struct event {
        hid_keycode_t keycode;
        keymod_t      mod;
    };

    static_assert(sizeof(event) == 2);
}