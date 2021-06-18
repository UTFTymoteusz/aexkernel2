#include "dev/tty/abstractor.hpp"

namespace AEX::Dev::TTY {
    Abstractor::Abstractor() {}

    Abstractor::Abstractor(VTTY* vtty) : m_vtty(vtty) {}

    int Abstractor::read() {
        return m_vtty->read();
    }

    TTY& Abstractor::write(char c) {
        _write(c);
        return *this;
    }

    TTY& Abstractor::write(const char* str) {
        size_t len = strlen(str);

        for (size_t i = 0; i < len; i++)
            _write(str[i]);

        return *this;
    }

    bool Abstractor::text() {
        return m_vtty->text();
    }

    bool Abstractor::graphics() {
        return m_vtty->graphics();
    }

    tty_info Abstractor::info() {
        return m_vtty->info();
    }

    void* Abstractor::output() {
        return m_vtty->output();
    }

    Proc::pid_t Abstractor::tcgetpgrp() {
        return m_vtty->tcgetpgrp();
    }

    void Abstractor::tcsetpgrp(Proc::pid_t pgrp) {
        m_vtty->tcsetpgrp(pgrp);
    }

    void Abstractor::_write(char c) {
        if (c == '\n')
            Sys::CPU::outb(0xE9, '\r');

        Sys::CPU::outb(0xE9, c);

        switch (m_state) {
        case OUT:
            if (c == '\x1B') {
                m_state = ANSI_CAT;
                break;
            }

            m_vtty->write(c);
            break;
        case ANSI_CAT:
            m_cat   = c;
            m_state = ANSI_ARGS;

            memset(m_buffer, '\0', sizeof(m_buffer));
            break;
        case ANSI_ARGS:
            if (!inrange(c, '0', '9')) {
                m_args[m_arg] = stoi<int>(10, m_buffer);

                m_ptr = 0;
                m_arg++;

                if (m_arg == 8)
                    m_arg--;

                memset(m_buffer, '\0', sizeof(m_buffer));

                if (inrange(c, ':', ';'))
                    break;

                m_code  = c;
                m_state = OUT;

                flush_ansi();
                break;
            }

            if (m_ptr == 31)
                m_ptr--;

            m_buffer[m_ptr++] = c;
            break;
        default:
            break;
        }
    }

    void Abstractor::flush_ansi() {
        switch (m_cat) {
        case '[':
            switch (m_code) {
            case 'm':
                m_vtty->color((ansi_color_t) m_args[0]);
                break;
            case 'H':
                m_vtty->setCursorX(m_args[0]);
                m_vtty->setCursorY(m_args[1]);
                break;
            default:
                break;
            }

            break;
        default:
            break;
        }

        m_ptr = 0;
        m_arg = 0;
    }
}