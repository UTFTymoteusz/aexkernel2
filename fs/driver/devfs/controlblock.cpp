#include "controlblock.hpp"

#include "inode.hpp"

namespace AEX::FS {
    optional<INode_SP> DevFSControlBlock::getINode(INode_SP, dir_entry, int id) {
        if (id == root_inode_id)
            return INode_SP(new DevFSRootINode());

        auto device = Dev::devices.get(id - 2);
        if (!device.isValid())
            return ENOENT;

        auto inode = new INode();

        inode->device_id = id - 2;

        switch (device->type) {
        case Dev::DEV_BLOCK:
            inode->type = FILE_BLOCK;
            break;
        case Dev::DEV_CHAR:
            inode->type = FILE_CHAR;
            break;
        case Dev::DEV_NET:
            inode->type = FILE_NET;
            break;
        }

        return INode_SP(inode);
    }
}