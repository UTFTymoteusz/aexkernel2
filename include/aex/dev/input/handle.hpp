#pragma once

#include "aex/dev/input/code.hpp"
#include "aex/dev/input/event.hpp"
#include "aex/mem/circularbuffer.hpp"

#include <stdint.h>

namespace AEX::Dev::Input {
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
}