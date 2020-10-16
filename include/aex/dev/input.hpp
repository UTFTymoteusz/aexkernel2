#pragma once

#include "aex/dev/input/code.hpp"
#include "aex/dev/input/event.hpp"
#include "aex/dev/input/handle.hpp"
#include "aex/dev/input/keymap.hpp"

namespace AEX::Dev::Input {
    extern keymap default_keymap;

    char translate(keymap* m_keymap, event& m_event);
}
