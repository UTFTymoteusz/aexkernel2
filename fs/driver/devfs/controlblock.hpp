#pragma once

#include "aex/fs/controlblock.hpp"
#include "aex/fs/directory.hpp"
#include "aex/fs/inode.hpp"
#include "aex/optional.hpp"

namespace AEX::FS {
    class DevFSControlBlock : public ControlBlock {
        public:
        optional<INode_SP> getINode(INode_SP dir, dir_entry dentry, int id);
    };
}