#include "fs/devfiles/charfile.hpp"

#include "aex/printk.hpp"

namespace AEX::FS {
    CharFile::CharFile(Dev::CharHandle_SP handle) : m_handle(handle) {}

    CharFile::~CharFile() {
        //
    }

    optional<ssize_t> CharFile::read(void* buf, size_t count) {
        return m_handle->read(buf, count);
    }

    optional<ssize_t> CharFile::write(void* buf, size_t count) {
        return m_handle->write(buf, count);
    }

    optional<int> CharFile::ioctl(int rq, uint64_t val) {
        return m_handle->ioctl(rq, val);
    }

    optional<Mem::MMapRegion*> CharFile::mmap(Proc::Process* process, void* addr, size_t len,
                                              int flags, FS::File_SP file, FS::off_t offset) {
        return m_handle->mmap(process, addr, len, flags, file, offset);
    }

    optional<File_SP> CharFile::dup() {
        auto dfile = new CharFile(m_handle);

        // Make this proper pls

        return File_SP(dfile);
    }

    bool CharFile::isatty() {
        return m_handle->isatty();
    }
}