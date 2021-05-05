#include "inodedirectory.hpp"

#include "aex/fs/directory.hpp"
#include "aex/fs/file.hpp"
#include "aex/fs/inode.hpp"
#include "aex/mem.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX::FS {
    INodeDirectory::INodeDirectory(INode_SP inode) {
        m_inode = inode;
    }

    optional<dirent> INodeDirectory::readdir() {
        SCOPE(m_inode->mutex);
        return m_inode->readDir(&m_dir_ctx);
    }

    error_t INodeDirectory::seekdir(long pos) {
        SCOPE(m_inode->mutex);
        return m_inode->seekDir(&m_dir_ctx, pos);
    }

    long INodeDirectory::telldir() {
        SCOPE(m_inode->mutex);
        return m_inode->tellDir(&m_dir_ctx);
    }

    optional<File_SP> INodeDirectory::dup() {
        SCOPE(m_inode->mutex);

        auto dupd       = new INodeDirectory(m_inode);
        dupd->m_dir_ctx = m_dir_ctx;

        return File_SP(dupd);
    }

    error_t INodeDirectory::close() {
        return ENONE;
    }
}