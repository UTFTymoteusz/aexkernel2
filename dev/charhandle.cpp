#include "aex/dev/charhandle.hpp"

#include "aex/dev/chardevice.hpp"

namespace AEX::Dev {
    CharHandle::CharHandle() {
        m_dev = CharDevice_SP::getNull();
    }

    CharHandle::CharHandle(CharDevice_SP chrdev) : m_dev(chrdev) {}

    CharHandle::~CharHandle() {
        if (m_dev.isValid())
            m_dev->close(this);
    }

    optional<ssize_t> CharHandle::read(void* ptr, size_t len) {
        return m_dev->read(this, ptr, len);
    }

    optional<ssize_t> CharHandle::write(const void* ptr, size_t len) {
        return m_dev->write(this, ptr, len);
    }

    bool CharHandle::isatty() {
        return m_dev->isatty();
    }
}
