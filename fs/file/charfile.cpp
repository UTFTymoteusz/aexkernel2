#include "charfile.hpp"

#include "aex/printk.hpp"

namespace AEX::FS {
    CharFile::CharFile(Dev::CharHandle_SP handle) : m_handle(handle) {}

    CharFile::~CharFile() {
        //
    }

    optional<ssize_t> CharFile::read(void* buf, size_t count) {
        return m_handle->read(buf, count);
    }

    optional<ssize_t> CharFile::write(const void* buf, size_t count) {
        return m_handle->write(buf, count);
    }

    optional<int> CharFile::ioctl(int rq, uint64_t val) {
        return m_handle->ioctl(rq, val);
    }

    optional<Mem::Region*> CharFile::mmap(Proc::Process* process, void* addr, size_t len, int flags,
                                          FS::File_SP file, FS::off_t offset) {
        return m_handle->mmap(process, addr, len, flags, file, offset);
    }

    bool CharFile::isatty() {
        return m_handle->isatty();
    }
}