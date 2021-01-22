#include "cdev.hpp"

#include "aex/dev/tty/tty.hpp"
#include "aex/types.hpp"

namespace AEX::Dev {
    TTYChar::TTYChar(int index, const char* name) : CharDevice(name) {
        m_index = index;
        m_tty   = TTY::TTYs[index];
    }

    error_t TTYChar::open(CharHandle* handle, int) {
        auto scope = m_mutex.scope();

        // m_stack.push(handle); // i need an insert() method
        m_current = handle;

        return ENONE;
    }

    error_t TTYChar::close(CharHandle* handle) {
        auto scope = m_mutex.scope();

        if (m_closed)
            return EINVAL;

        m_closed = true;

        for (int i = 0; i < m_stack.count(); i++) {
            if (m_stack[i] != handle)
                continue;

            m_stack.erase(i);
            break;
        }

        m_current = m_stack.count() ? m_stack[0] : nullptr;

        return ENONE;
    }

    optional<ssize_t> TTYChar::read(CharHandle*, void* ptr, size_t len) {
        auto cptr = (char*) ptr;

        for (size_t i = 0; i < len; i++)
            cptr[i] = m_tty->read();

        return len;
    }

    optional<ssize_t> TTYChar::write(CharHandle*, const void* ptr, size_t len) {
        auto cptr = (char*) ptr;

        for (size_t i = 0; i < len; i++)
            m_tty->write(cptr[i]);

        return len;
    }

    bool TTYChar::isatty() {
        return true;
    }
}