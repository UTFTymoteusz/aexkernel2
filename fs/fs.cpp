#include "aex/fs.hpp"

#include "aex/errno.hpp"
#include "aex/fs/mount.hpp"
#include "aex/math.hpp"
#include "aex/mem.hpp"
#include "aex/printk.hpp"
#include "aex/string.hpp"

#include "fs/driver/devfs/devfs.hpp"
#include "fs/fs.hpp"

namespace AEX::FS {
    Mem::SmartArray<Filesystem> filesystems;
    Mem::SmartArray<Mount>      mounts;

    void init() {
        printk(PRINTK_INIT "fs: Initializing\n");

        DevFS::init();

        printk(PRINTK_OK "fs: Initialized\n");
    }

    error_t mount(const char* source, const char* path, const char* type) {
        if (!path || !is_valid(path))
            return EINVAL;

        if (source && !is_valid(source))
            return EINVAL;

        for (auto iterator = filesystems.getIterator(); auto fs = iterator.next();) {
            if (type && strcmp(type, fs->name) != 0)
                continue;

            auto res = fs->mount(source);
            if (!res)
                continue;

            auto mount = new Mount();

            printk(PRINTK_OK "fs: Mounted '%s' at %s\n", fs->name, path);
            strncpy(mount->path, path, sizeof(mount->path));
            mount->control_block = res.value;

            mounts.addRef(mount);
            return ENONE;
        }

        return EINVAL;
    }

    mount_info find_mount(const char* path) {
        if (!path || !is_valid(path))
            return mount_info(optional<Mem::SmartPointer<Mount>>::error(EINVAL), nullptr);

        Mem::SmartPointer<Mount> ret;

        bool found   = false;
        int  max_len = 0;

        for (auto iterator = mounts.getIterator(); iterator.next();) {
            auto mount = iterator.get_ptr();

            int mnt_len  = strlen(mount->path) - 1;
            int path_len = max(strlen(path) - (ends_with_slash(path) ? 1 : 0), 1);

            if (path_len < mnt_len)
                continue;

            if (memcmp(mount->path, path, mnt_len) != 0)
                continue;

            if (mnt_len < max_len)
                continue;

            max_len = mnt_len;

            ret   = mount;
            found = true;
        }

        if (found) {
            const char* new_path = path + max_len;
            if (new_path[0] == '\0')
                new_path = "/";

            return mount_info(ret, new_path);
        }

        return mount_info(optional<Mem::SmartPointer<Mount>>::error(ENOENT), nullptr);
    }
}