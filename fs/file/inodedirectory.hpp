#pragma once

#include "aex/fs/directory.hpp"
#include "aex/fs/file.hpp"
#include "aex/fs/inode.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX::FS {
    class INodeDirectory : public File {
        public:
        INodeDirectory(INode_SP inode);

        optional<dirent> readdir();
        error_t          seekdir(long pos);
        long             telldir();
        error_t          close();

        private:
        dir_context m_dir_ctx = dir_context();

        Mem::SmartPointer<INode> m_inode;
    };
}