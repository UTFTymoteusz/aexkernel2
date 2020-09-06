#include "controlblock.hpp"

#include "inode.hpp"

namespace AEX::FS {
    optional<INode_SP> DevFSControlBlock::getINode(INode_SP, dir_entry, int id) {
        if (id == root_inode_id)
            return INode_SP(new DevFSRootINode());

        auto device = Dev::devices.get(id - 2);
        if (!device)
            return ENOENT;

        auto inode = new INode();

        inode->device_id = id - 2;

        switch (device->type) {
        case Dev::DEV_BLOCK:
            inode->type = FT_BLOCK;
            break;
        case Dev::DEV_CHAR:
            inode->type = FT_CHAR;
            break;
        case Dev::DEV_NET:
            inode->type = FT_NET;
            break;
        }

        return INode_SP(inode);
    }
}