#include "aex/fs/mount.hpp"

#include "aex/printk.hpp"

namespace AEX::FS {
    Mount::~Mount() {
        printk("mount deleted\n");
    }

    void Mount::unmount() {}

    optional<Mem::SmartPointer<File>> Mount::opendir(const char*) {
        return ENOSYS;
    }
}