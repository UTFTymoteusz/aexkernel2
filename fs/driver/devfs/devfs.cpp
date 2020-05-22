#include "devfs.hpp"

#include "aex/dev/block.hpp"
#include "aex/printk.hpp"

namespace AEX::FS {
    void DevFS::init() {
        new DevFS();
    }

    DevFS::DevFS() : Filesystem("devfs") {}
    DevFS::~DevFS() {}

    optional<Mount*> DevFS::mount(const char* source) {
        if (source)
            return {};

        return new DevFSMount();
    }
}