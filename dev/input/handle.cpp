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

    event Handle::readEvent() {
        event m_evnt;
        m_buffer.read(&m_evnt, sizeof(event));
        return m_evnt;
    }

    void Handle::writeEvent(event m_evnt) {
        if ((size_t) m_buffer.writeAvailable() < sizeof(event))
            return;

        m_buffer.write(&m_evnt, sizeof(event));
    }
}