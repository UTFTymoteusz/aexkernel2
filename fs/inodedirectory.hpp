#pragma once

#include "aex/fs/directory.hpp"
#include "aex/fs/file.hpp"
#include "aex/fs/inode.hpp"
#include "aex/mem.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX::FS {
    class INodeDirectory : public File {
        public:
        INodeDirectory(INode_SP inode) {
            m_inode = inode;
        }

        optional<dir_entry> readdir() {
            return m_inode->readDir(&m_dir_ctx);
        }

        error_t seekdir(long pos) {
            return m_inode->seekDir(&m_dir_ctx, pos);
        }

        long telldir() {
            return m_inode->tellDir(&m_dir_ctx);
        }

        optional<File_SP> dup() {
            auto dupd = new INodeDirectory(m_inode);

            dupd->m_dir_ctx = m_dir_ctx;

            return File_SP(dupd);
        }

        private:
        dir_context m_dir_ctx = dir_context();

        Mem::SmartPointer<INode> m_inode;
    };
}