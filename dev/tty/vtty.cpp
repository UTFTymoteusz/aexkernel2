#include "aex/dev/tty/vtty.hpp"

#include "aex/arch/sys/cpu.hpp"
#include "aex/dev/tty.hpp"
#include "aex/mem.hpp"
#include "aex/printk.hpp"
#include "aex/proc.hpp"
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
        m_inputBuffer = new Mem::CircularBuffer<int, true>(256);
    }

    void VTTY::keyPress(event m_event) {
        if (!m_inputBuffer->writeav())
            return;

        if (m_event.mod & KEYMOD_CTRL &&
            (inrange(m_event.keycode, KEY_A, KEY_Z) || m_event.keycode == KEY_BACKSLASH)) {
            switch (m_event.keycode) {
            case KEY_C: {
                SCOPE(m_mutex);

                if (m_foreground_pgrp != -1)
                    Proc::Process::kill(-m_foreground_pgrp, IPC::SIGINT);
            }
                return;
            case KEY_D:
                m_inputBuffer->write(-1);
                return;
            case KEY_X: {
                SCOPE(m_mutex);

                if (m_foreground_pgrp != -1)
                    Proc::Process::kill(-m_foreground_pgrp, IPC::SIGCONT);
            }
                return;
            case KEY_Y:
                m_inputBuffer->write(-2);
                return;
            case KEY_Z: {
                SCOPE(m_mutex);

                if (m_foreground_pgrp != -1)
                    Proc::Process::kill(-m_foreground_pgrp, IPC::SIGTSTP);
            }
                return;
            case KEY_BACKSLASH:
            case KEY_BACKSLASH2: {
                SCOPE(m_mutex);

                if (m_foreground_pgrp != -1)
                    Proc::Process::kill(-m_foreground_pgrp, IPC::SIGQUIT);
            }
                return;
            default:
                break;
            }

            m_inputBuffer->write(1 + (m_event.keycode - KEY_A));
            return;
        }

        char c = translate(&m_keymap, m_event);
        if (!c)
            return;

        m_inputBuffer->write((int) c);
    }
}