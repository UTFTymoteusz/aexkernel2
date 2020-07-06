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
        case Dev::dev_type_t::DEV_BLOCK:
            inode->type = fs_type_t::BLOCK;
            break;
        case Dev::dev_type_t::DEV_CHAR:
            inode->type = fs_type_t::CHAR;
            break;
        case Dev::dev_type_t::DEV_NET:
            inode->type = fs_type_t::NET;
            break;
        }

        return INode_SP(inode);
    }
}