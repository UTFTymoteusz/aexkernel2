#include "aex/fs/fs.hpp"

#include "aex/errno.hpp"
#include "aex/fs/mount.hpp"
#include "aex/math.hpp"
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

    void init() {
        printk(PRINTK_INIT "fs: Initializing\n");

        DevFS::init();
        ISO9660::init();

        printk(PRINTK_OK "fs: Initialized\n");
    }

    error_t mount(const char* source, const char* path, const char* type) {
        if (!path || !Path::is_valid(path))
            return error_t::EINVAL;

        if (source && !Path::is_valid(source))
            return error_t::EINVAL;

        for (auto iterator = filesystems.getIterator(); auto fs = iterator.next();) {
            if (type && strcmp(type, fs->name) != 0)
                continue;

            auto res = fs->mount(source);
            if (!res.has_value)
                continue;

            auto mount = new Mount();

            printk(PRINTK_OK "fs: Mounted '%s' at %s\n", fs->name, path);
            strncpy(mount->path, path, sizeof(mount->path));
            mount->control_block = res.value;

            mounts.addRef(mount);
            return error_t::ENONE;
        }

        return error_t::EINVAL;
    }

    mount_info find_mount(const char* path) {
        if (!path || !Path::is_valid(path))
            return mount_info(optional<Mem::SmartPointer<Mount>>::error(error_t::EINVAL), nullptr);

        Mem::SmartPointer<Mount> ret;

        bool found   = false;
        int  max_len = 0;

        for (auto iterator = mounts.getIterator(); iterator.next();) {
            auto mount = iterator.get_ptr();

            int mnt_len  = strlen(mount->path) - 1;
            int path_len = max(strlen(path) - (Path::ends_with_slash(path) ? 1 : 0), 1);

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