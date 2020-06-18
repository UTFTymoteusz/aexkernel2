#pragma once

#include "aex/fs/directory.hpp"
#include "aex/fs/file.hpp"
#include "aex/fs/inode.hpp"
#include "aex/mem/smartptr.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX::FS {
    class INodeDirectory : public File {
        public:
        INodeDirectory(INode_SP inode) {
            _inode = inode;
        }

        optional<dir_entry> readdir() {
            return _inode->readDir(&dir_ctx);
        }

        private:
        dir_context dir_ctx = dir_context();

        Mem::SmartPointer<INode> _inode;
    };
}