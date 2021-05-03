#pragma once

#include "aex/fs/controlblock.hpp"
#include "aex/fs/directory.hpp"
#include "aex/fs/inode.hpp"
#include "aex/optional.hpp"

namespace AEX::FS {
    class DevFSControlBlock : public ControlBlock {
        public:
        optional<INode_SP> get(INode_SP dir, dirent dentry, ino_t id);
    };
}