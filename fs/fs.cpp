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
    char* zero;

    Mem::SmartArray<Filesystem> filesystems;
    Mem::SmartArray<Mount>      mounts;

    void init() {
        printk(INIT "fs: Initializing\n");
        zero = new char[8192];

        DevFS::init();

        printk(OK "fs: Initialized\n");
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

            printk(OK "fs: Mounted '%s' at %s\n", fs->name, path);
            strlcpy(mount->path, path, sizeof(mount->path));
            mount->controlblock       = res.value;
            mount->controlblock->path = mount->path;

            mounts.addRef(mount);
            return ENONE;
        }

        return EINVAL;
    }

    optional<mount_info> find_mount(const char* path) {
        if (!path || !is_valid(path))
            return EINVAL;

        Mem::SmartPointer<Mount> ret;

        bool   found   = false;
        size_t max_len = 0;

        for (auto iterator = mounts.getIterator(); iterator.next();) {
            auto mount = iterator.get_ptr();

            size_t mnt_len  = strlen(mount->path) - 1;
            size_t path_len = max<size_t>(strlen(path) - (ends_with_slash(path) ? 1 : 0), 1);

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

        if (!found)
            return ENOENT;

        const char* new_path = path + max_len;
        if (new_path[0] == '\0')
            new_path = "/";

        return mount_info(ret, new_path);
    }
}