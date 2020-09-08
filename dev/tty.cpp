#include "aex/dev/tty.hpp"

#include "aex/arch/sys/cpu.hpp"
#include "aex/mem.hpp"
#include "aex/printk.hpp"
#include "aex/string.hpp"

#include <stddef.h>
#include <stdint.h>

using namespace AEX::Dev::Input;

namespace AEX::Dev::TTY {
    VTTY::VTTY() {}
    VTTY::VTTY(int width, int height) {
        this->width  = width;
        this->height = height;
    }

    void VTTY::inputReady() {
        m_inputBuffer = new Mem::CircularBuffer(2048);
    }

    void VTTY::inputKeyPress(event m_event) {
        if (!m_inputBuffer->writeAvailable())
            return;

        if (m_event.mod & KEYMOD_CTRL && m_event.keycode >= KEY_A && m_event.keycode <= KEY_Z) {
            char cc = 1 + (m_event.keycode - KEY_A);
            m_inputBuffer->write(&cc, 1);

            return;
        }

        char c = translateEvent(&m_keymap, m_event);
        if (!c)
            return;

        m_inputBuffer->write(&c, 1);
    }
}