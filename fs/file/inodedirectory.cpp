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
        return m_inode->readdir(&m_dir_ctx);
    }

    error_t INodeDirectory::seekdir(long pos) {
        SCOPE(m_inode->mutex);
        return m_inode->seekdir(&m_dir_ctx, pos);
    }

    long INodeDirectory::telldir() {
        SCOPE(m_inode->mutex);
        return m_inode->telldir(&m_dir_ctx);
    }

    error_t INodeDirectory::close() {
        return ENONE;
    }
}