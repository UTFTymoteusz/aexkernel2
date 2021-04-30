#pragma once

#include "aex/dev/input/code.hpp"
#include "aex/dev/input/event.hpp"
#include "aex/dev/input/handle.hpp"
#include "aex/dev/input/keymap.hpp"
#include "aex/utility.hpp"

namespace AEX::Dev::Input {
    API extern keymap default_keymap;

    /**
     * Translates a event into a character.
     * @param m_keymap Pointer to a keymap.
     * @param m_event Reference to the event.
     * @returns The translated character.
     **/
    API char translate(keymap* m_keymap, event& m_event);
}
