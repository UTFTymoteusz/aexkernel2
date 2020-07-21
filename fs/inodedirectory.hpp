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
            _inode = inode;
        }

        optional<dir_entry> readdir() {
            return _inode->readDir(&_dir_ctx);
        }

        optional<File_SP> dup() {
            auto dupd = new INodeDirectory(_inode);

            dupd->_dir_ctx = _dir_ctx;

            return File_SP(dupd);
        }

        private:
        dir_context _dir_ctx = dir_context();

        Mem::SmartPointer<INode> _inode;
    };
}