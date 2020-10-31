#include "cdev.hpp"

#include "aex/dev/tty.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX::Dev {
    TTYChar::TTYChar(int index, const char* name) : CharDevice(name) {
        m_index = index;
        m_vtty  = TTY::VTTYs[index];
    }

    error_t TTYChar::open(CharHandle* handle, int) {
        auto scope = m_mutex.scope();

        // m_stack.push(handle); // i need an insert() method
        m_current = handle;

        return ENONE;
    }

    error_t TTYChar::close(CharHandle* handle) {
        auto scope = m_mutex.scope();

        for (int i = 0; i < m_stack.count(); i++) {
            if (m_stack[i] != handle)
                continue;

            m_stack.erase(i);
            break;
        }

        m_current = m_stack.count() ? m_stack[0] : nullptr;

        return ENONE;
    }

    optional<uint32_t> TTYChar::read(CharHandle*, void*, uint32_t) {
        return EBOTHER;
    }

    optional<uint32_t> TTYChar::write(CharHandle*, const void* ptr, uint32_t len) {
        auto cptr = (char*) ptr;

        for (uint32_t i = 0; i < len; i++)
            m_vtty->write(cptr[i]);

        return len;
    }
}