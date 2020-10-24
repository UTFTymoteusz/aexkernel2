#include "fs/devfiles/charfile.hpp"

#include "aex/printk.hpp"

namespace AEX::FS {
    CharFile::CharFile(Dev::CharHandle_SP handle) : m_handle(handle) {}

    CharFile::~CharFile() {
        //
    }

    optional<uint32_t> CharFile::read(void* buf, uint32_t count) {
        return m_handle->read(buf, count);
    }

    optional<uint32_t> CharFile::write(void* buf, uint32_t count) {
        return m_handle->write(buf, count);
    }
}