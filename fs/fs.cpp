#include "aex/fs/fs.hpp"

#include "aex/fs/mount.hpp"
#include "aex/mem/smartarray.hpp"
#include "aex/printk.hpp"
#include "aex/string.hpp"

#include "fs/driver/iso9660/iso9660.hpp"
#include "fs/fs.hpp"

namespace AEX::FS {
    Mem::SmartArray<Filesystem> filesystems;
    Mem::SmartArray<Mount>      mounts;

    void init() {
        printk(PRINTK_INIT "fs: Initializing\n");

        ISO9660::init();

        printk(PRINTK_OK "fs: Initialized\n");
    }

    int mount(const char* source, const char* path, const char* type) {
        for (auto iterator = filesystems.getIterator(); auto fs = iterator.next();) {
            if (type && strcmp(type, fs->name) != 0)
                continue;

            auto res = fs->mount(source);
            if (!res.has_value)
                continue;

            mounts.addRef(res.value);
            return 0;
        }

        return -1;
    }
}