#include "controlblock.hpp"

#include "inode.hpp"

namespace AEX::FS {
    optional<INode_SP> DevFSControlBlock::getINode(INode_SP dir, dir_entry, int id) {
        if (id == root_inode_id)
            return INode_SP(new DevFSRootINode());

        auto device = Dev::devices.get(id - 2);
        if (!device.isValid())
            return error_t::ENOENT;

        auto inode = new INode();

        inode->device_id = id - 2;

        switch (device->type) {
        case Dev::Device::type_t::BLOCK:
            inode->type = fs_type_t::BLOCK;
            break;
        case Dev::Device::type_t::CHAR:
            inode->type = fs_type_t::CHAR;
            break;
        case Dev::Device::type_t::NET:
            inode->type = fs_type_t::NET;
            break;
        }

        return INode_SP(inode);
    }
}