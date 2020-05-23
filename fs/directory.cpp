#include "aex/fs/directory.hpp"

#include "aex/errno.hpp"

#include "fs/fs.hpp"

namespace AEX::FS {
    optional<Mem::SmartPointer<Directory>> Directory::open(const char* path) {
        auto mount_info = find_mount(path);
        if (!mount_info.mount.has_value)
            return optional<Mem::SmartPointer<Directory>>::error(mount_info.mount.error_code);

        return mount_info.mount.value->opendir(path);
    }
}