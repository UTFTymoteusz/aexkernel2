#include "aex/dev/tty.hpp"

namespace AEX::Dev::TTY {
    void VTTY::_write(char) {
        kpanic("VTTY::_write() not implemented");
    }

    VTTY& VTTY::scroll(int) {
        kpanic("VTTY::scrollDown() not implemented");
    }

    VTTY& VTTY::color(ansi_color_t) {
        kpanic("VTTY::setColorANSI() not implemented");
    }

    char VTTY::read() {
        char c;
        m_inputBuffer->read(&c, 1);

        return c;
    }

    VTTY& VTTY::write(char c) {
        m_lock.acquire();
        Sys::CPU::outportb(0xE9, c);
        _write(c);
        m_lock.release();

        return *this;
    }

    VTTY& VTTY::write(const char* str) {
        m_lock.acquire();

        while (*str != '\0') {
            Sys::CPU::outportb(0xE9, *str);
            _write(*str++);
        }

        m_lock.release();
        return *this;
    }
}
