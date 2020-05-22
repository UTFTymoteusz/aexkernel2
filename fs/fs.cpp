#include "aex/fs/fs.hpp"

#include "aex/errno.hpp"
#include "aex/fs/mount.hpp"
#include "aex/mem/smartarray.hpp"
#include "aex/mem/smartptr.hpp"
#include "aex/printk.hpp"
#include "aex/string.hpp"

#include "fs/driver/devfs/devfs.hpp"
#include "fs/driver/iso9660/iso9660.hpp"
#include "fs/fs.hpp"

namespace AEX::FS {
    Mem::SmartArray<Filesystem> filesystems;
    Mem::SmartArray<Mount>      mounts;

    Spinlock mount_lock;

    void init() {
        printk(PRINTK_INIT "fs: Initializing\n");

        DevFS::init();
        ISO9660::init();

        printk(PRINTK_OK "fs: Initialized\n");
    }

    int mount(const char* source, const char* path, const char* type) {
        if (!path || !Path::is_valid(path))
            return error::EINVAL;

        if (source && !Path::is_valid(source))
            return error::EINVAL;

        auto scopeLock = ScopeSpinlock(mount_lock);

        for (auto iterator = filesystems.getIterator(); auto fs = iterator.next();) {
            if (type && strcmp(type, fs->name) != 0)
                continue;

            auto res = fs->mount(source);
            if (!res.has_value)
                continue;

            printk(PRINTK_OK "fs: Mounted '%s' at %s\n", fs->name, path);
            strncpy(res.value->path, path, sizeof(res.value->path));

            mounts.addRef(res.value);
            return 0;
        }

        return error::EINVAL;
    }

    optional<Mem::SmartPointer<Mount>> find_mount(char* path) {
        if (!path || !Path::is_valid(path))
            return optional<Mem::SmartPointer<Mount>>::error(error::EINVAL);

        auto scopeLock = ScopeSpinlock(mount_lock);

        Mem::SmartPointer<Mount> ret;

        bool found   = false;
        int  max_len = 0;

        for (auto iterator = mounts.getIterator(); iterator.next();) {
            auto mount = iterator.get_ptr();

            printk("aaa path: %s\n", mount->path);

            int mnt_len  = strlen(mount->path);
            int path_len = strlen(path) + Path::ends_with_slash(path) ? 0 : 1;


            if (memcmp(mount->path, path, strlen(path)))
                ;
        }

        return optional<Mem::SmartPointer<Mount>>::error(ENOENT);
    }
}