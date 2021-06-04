#include "aex/dev/tty.hpp"
#include "aex/dev/tty/vtty.hpp"

namespace AEX::Dev::TTY {
    int VTTY::read() {
        int c;
        if (m_inputBuffer->read(&c, 1) == 0)
            return -1;

        return c;
    }

    VTTY& VTTY::write(char c) {
        m_lock.acquire();
        _write(c);
        m_lock.release();

        return *this;
    }

    VTTY& VTTY::write(const char* str) {
        m_lock.acquire();

        while (*str != '\0') {
            if (*str == '\n')
                Sys::CPU::outb(0xE9, '\r');

            Sys::CPU::outb(0xE9, *str);
            _write(*str++);
        }

        m_lock.release();
        return *this;
    }
}
