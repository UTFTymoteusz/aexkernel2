#include "aex/fs/file.hpp"

#include "aex/dev.hpp"
#include "aex/fs/inode.hpp"
#include "aex/kpanic.hpp"
#include "aex/printk.hpp"
#include "aex/string.hpp"

#include "fs/devfile.hpp"
#include "fs/file/inodedirectory.hpp"
#include "fs/file/inodefile.hpp"
#include "fs/fs.hpp"

namespace AEX::FS {
    dirent::dirent(const char* name, int pos, int inode_id) {
        strncpy(this->name, name, sizeof(this->name));
        this->inode_id = inode_id;
        this->pos      = pos;
    }

    File::~File() {
        close();
    }

    optional<File_SP> File::open(const char* path, int mode) {
        auto mount_info = ENSURE_OPT(find_mount(path));
        auto inode_try  = mount_info.mount->control_block->find(mount_info.new_path);
        if (!inode_try) {
            PRINTK_DEBUG3("%s, %i: no inode (%s)", path, mode, strerror(inode_try.error));
            return inode_try.error;
        }

        auto inode = inode_try.value;
        if (inode->is_directory()) {
            if (mode != O_RD)
                return EISDIR;

            return File_SP(new INodeDirectory(inode));
        }

        if (inode->dev != -1) {
            auto device = ENSURE_R(Dev::devices.get(inode->dev), ENOENT);
            auto file   = ENSURE_OPT(DevFile::open(device, mode));

            return File_SP(file);
        }

        return File_SP(new INodeFile(inode, mode));
    }

    optional<file_info> File::info(const char* path, int) {
        auto mount_info = ENSURE_OPT(find_mount(path));
        auto inode_try  = mount_info.mount->control_block->find(mount_info.new_path);
        if (!inode_try)
            return inode_try.error;

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

    optional<ssize_t> File::write(const void*, size_t) {
        return ENOSYS;
    }

    optional<int> File::ioctl(int, uint64_t) {
        return ENOSYS;
    }

    optional<Mem::Region*> File::mmap(Proc::Process*, void*, size_t, int, FS::File_SP, FS::off_t) {
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

    optional<dirent> File::readdir() {
        return ENOTDIR;
    }

    error_t File::seekdir(long) {
        return ENOTDIR;
    }

    long File::telldir() {
        return -1;
    }

    optional<File_SP> File::dup() {
        kpanic("Attempt to call the default dup()");
    }

    error_t File::close() {
        return ENONE;
    }

    bool File::isatty() {
        return false;
    }

    int File::get_flags() {
        return m_flags;
    }

    void File::set_flags(int flags) {
        m_flags = flags;
    }
}