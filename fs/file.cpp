#include "aex/fs/file.hpp"

#include "aex/fs/inode.hpp"
#include "aex/kpanic.hpp"
#include "aex/printk.hpp"
#include "aex/string.hpp"

#include "fs/devfile.hpp"
#include "fs/fs.hpp"
#include "fs/inodedirectory.hpp"
#include "fs/inodefile.hpp"

namespace AEX::FS {
    dir_entry::dir_entry(const char* name, int pos, int inode_id) {
        strncpy(this->name, name, sizeof(this->name));
        this->inode_id = inode_id;
        this->pos      = pos;
    }

    File::~File() {}

    optional<File_SP> File::open(const char* path) {
        auto mount_info = find_mount(path);
        if (!mount_info.mount)
            return mount_info.mount.error_code;

        auto inode_try = mount_info.mount.value->control_block->findINode(mount_info.new_path);
        if (!inode_try)
            return inode_try.error_code;

        auto inode = inode_try.value;
        if (inode->is_directory())
            return EISDIR;

        if (inode->device_id != -1) {
            auto device = Dev::devices.get(inode->device_id);
            if (!device)
                return ENOENT;

            return File_SP(new DevFile(device));
        }

        return File_SP(new INodeFile(inode));
    }

    optional<File_SP> File::opendir(const char* path) {
        auto mount_info = find_mount(path);
        if (!mount_info.mount)
            return mount_info.mount.error_code;

        auto inode_try = mount_info.mount.value->control_block->findINode(mount_info.new_path);
        if (!inode_try)
            return inode_try.error_code;

        auto inode = inode_try.value;
        if (!inode->is_directory())
            return ENOTDIR;

        return File_SP(new INodeDirectory(inode));
    }

    optional<file_info> File::info(const char* path) {
        auto mount_info = find_mount(path);
        if (!mount_info.mount)
            return mount_info.mount.error_code;

        auto inode_try = mount_info.mount.value->control_block->findINode(mount_info.new_path);
        if (!inode_try)
            return inode_try.error_code;

        auto inode = inode_try.value;
        auto finfo = file_info();

        finfo.containing_dev_id = 0;
        finfo.type              = inode->type;
        finfo.dev_id            = inode->device_id;
        finfo.total_size        = inode->size;

        return finfo;
    }

    optional<uint32_t> File::read(void*, uint32_t) {
        return ENOSYS;
    }

    optional<uint32_t> File::write(void*, uint32_t) {
        return ENOSYS;
    }

    optional<int64_t> File::seek(int64_t, seek_mode) {
        return ENOSYS;
    }

    optional<dir_entry> File::readdir() {
        return ENOTDIR;
    }

    optional<File_SP> File::dup() {
        kpanic("Attempt to call the default dup()");
    }

    error_t File::close() {
        return ENOSYS;
    }
}