#include "aex/dev/input.hpp"

#include "dev/input.hpp"

namespace AEX::Dev::Input {
    Handle::Handle(int buffer_size) : m_buffer(buffer_size) {}

    Handle::~Handle() {
        unregister_handle(this);
    }

    Handle Handle::getHandle(int buffer_size) {
        return Handle(buffer_size);
    }

    void Handle::begin() {
        if (m_registered)
            return;

        m_registered = true;

        register_handle(this);
    }

    event Handle::read() {
        event m_evnt;
        m_buffer.read(&m_evnt, 1);
        return m_evnt;
    }

    void Handle::write(event m_evnt) {
        if (!m_buffer.writeav())
            return;

        m_buffer.write(&m_evnt, 1);
    }
}