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

    File::~File() {
        close();
    }

    optional<File_SP> File::open(const char* path, int mode) {
        auto mount_info = find_mount(path);
        if (!mount_info.mount)
            return mount_info.mount.error_code;

        auto inode_try = mount_info.mount.value->control_block->findINode(mount_info.new_path);
        if (!inode_try)
            return inode_try.error_code;

        auto inode = inode_try.value;
        if (inode->is_directory())
            return EISDIR;

        if (inode->dev != -1) {
            auto device = Dev::devices.get(inode->dev);
            if (!device)
                return ENOENT;

            auto dev_try = DevFile::open(device, mode);
            if (!dev_try)
                return dev_try.error_code;

            return File_SP(dev_try.value);
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

    optional<file_info> File::info(const char* path, int) {
        auto mount_info = find_mount(path);
        if (!mount_info.mount)
            return mount_info.mount.error_code;

        auto inode_try = mount_info.mount.value->control_block->findINode(mount_info.new_path);
        if (!inode_try)
            return inode_try.error_code;

        auto inode = inode_try.value;
        auto finfo = file_info();

        finfo.containing_dev_id = 0;
        finfo.inode             = inode->id;
        finfo.type              = inode->type;
        finfo.mode              = inode->mode;
        finfo.hard_links        = inode->hard_links;

        finfo.uid = inode->uid;
        finfo.gid = inode->gid;
        finfo.dev = inode->dev;

        finfo.access_time = inode->access_time;
        finfo.modify_time = inode->modify_time;
        finfo.change_time = inode->change_time;

        finfo.blocks     = inode->block_count;
        finfo.block_size = inode->block_size;
        finfo.total_size = inode->size;

        return finfo;
    }

    optional<ssize_t> File::read(void*, size_t) {
        return ENOSYS;
    }

    optional<ssize_t> File::write(void*, size_t) {
        return ENOSYS;
    }

    optional<file_info> File::finfo() {
        kpanic("Attempt to call the default finfo()");
    }

    error_t File::fchmod(mode_t) {
        kpanic("Attempt to call the default fchmod()");
    }

    optional<int64_t> File::seek(int64_t, seek_mode) {
        return ESPIPE;
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

    bool File::isatty() {
        return false;
    }
}