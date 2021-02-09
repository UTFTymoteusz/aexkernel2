#pragma once

#include "aex/dev/charhandle.hpp"
#include "aex/fs/file.hpp"
#include "aex/types.hpp"

namespace AEX::FS {
    class CharFile : public File {
        public:
        CharFile(Dev::CharHandle_SP handle);
        ~CharFile();

        optional<ssize_t>          read(void* buf, size_t count);
        optional<ssize_t>          write(void* buf, size_t count);
        optional<int>              ioctl(int rq, uint64_t val);
        optional<Mem::MMapRegion*> mmap(Proc::Process* process, void*, size_t len, int flags,
                                        FS::File_SP file, FS::off_t offset);


        optional<File_SP> dup();

        bool isatty();

        private:
        Dev::CharHandle_SP m_handle;
    };
}
