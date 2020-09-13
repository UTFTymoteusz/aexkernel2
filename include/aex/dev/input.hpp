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

        event read();

        private:
        Mem::CircularBuffer m_buffer;
        bool                m_registered = false;

        Handle(int buffer_size);

        void write(event evnt);

        friend void key_press(hid_keycode_t code);
        friend void key_release(hid_keycode_t code);
    };

    extern keymap default_keymap;

    char translate(keymap* m_keymap, event& m_event);
}
