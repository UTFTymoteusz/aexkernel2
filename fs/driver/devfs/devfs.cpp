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

        auto controlblock = new DevFSControlBlock();

        controlblock->root_inode_id = 1;

        return controlblock;
    }
}