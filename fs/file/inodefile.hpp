#pragma once

#include "aex/fs/file.hpp"
#include "aex/types.hpp"

namespace AEX::FS {
    class INodeFile : public File {
        public:
        INodeFile(INode_SP inode, int mode);
        ~INodeFile();

        optional<ssize_t>      read(void* buffer, size_t count);
        optional<ssize_t>      write(const void* buf, size_t count);
        optional<off_t>        seek(off_t offset, seek_mode mode);
        error_t                close();
        optional<file_info>    finfo();
        optional<Mem::Region*> mmap(Proc::Process* process, void*, size_t len, int flags,
                                    FS::File_SP file, FS::off_t offset);

        private:
        Mem::SmartPointer<INode> m_inode;
        int                      m_mode;

        off_t m_pos = 0;

        error_t readBlocks(void* buffer, off_t start, size_t len);
        error_t writeBlocks(const void* buffer, off_t start, size_t len);
        bool    isPerfect(off_t start, size_t len);
    };
}