#include "devfs.hpp"

#include "controlblock.hpp"
#include "inode.hpp"

namespace AEX::FS {
    DevFS* devfs;

    void DevFS::init() {
        devfs = new DevFS();
    }

    DevFS::DevFS() : Filesystem("devfs") {}
    DevFS::~DevFS() {}

    optional<ControlBlock*> DevFS::mount(const char* source) {
        if (source)
            return EINVAL;

        auto control_block = new DevFSControlBlock();

        control_block->root_inode_id = 1;

        return control_block;
    }
}