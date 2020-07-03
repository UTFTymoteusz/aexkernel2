#pragma once

#include "aex/dev/input/code.hpp"
#include "aex/dev/input/keymap.hpp"
#include "aex/mem/circularbuffer.hpp"

namespace AEX::Dev::Input {
    struct event {
        hid_keycode_t keycode;
        keymod_t      mod;
    };

    static_assert(sizeof(event) == 2);

    class Handle {
        public:
        Handle() = delete;
        ~Handle();

        static Handle getHandle(int buffer_size = 256);

        void begin();

        event readEvent();

        private:
        Mem::CircularBuffer _buffer;
        bool                _registered = false;

        Handle(int buffer_size);

        void writeEvent(event evnt);

        friend void key_press(hid_keycode_t code);
        friend void key_release(hid_keycode_t code);
    };

    char translateEvent(keymap* _keymap, event& _event);
}
