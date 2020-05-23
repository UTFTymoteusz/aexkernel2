#include "aex/fs/mount.hpp"

#include "aex/printk.hpp"

namespace AEX::FS {
    Mount::~Mount() {
        printk("mount deleted\n");
    }

    void Mount::unmount() {}

    optional<Mem::SmartPointer<Directory>> Mount::opendir(const char*) {
        return optional<Mem::SmartPointer<Directory>>::error(ENOSYS);
    }
}