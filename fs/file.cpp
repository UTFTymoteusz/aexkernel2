#include "aex/fs/file.hpp"

#include "aex/string.hpp"

#include "fs/fs.hpp"

namespace AEX::FS {
    dir_entry::dir_entry(const char* name) {
        strncpy(this->name, name, sizeof(this->name));
    }

    File::~File() {}

    optional<Mem::SmartPointer<File>> File::open(const char* path) {
        auto mount_info = find_mount(path);
        if (!mount_info.mount.has_value)
            return mount_info.mount.error_code;

        return mount_info.mount.value->open(mount_info.new_path);
    }

    optional<Mem::SmartPointer<File>> File::opendir(const char* path) {
        auto mount_info = find_mount(path);
        if (!mount_info.mount.has_value)
            return mount_info.mount.error_code;

        return mount_info.mount.value->opendir(mount_info.new_path);
    }

    optional<file_info> File::info(const char* path) {
        auto mount_info = find_mount(path);
        if (!mount_info.mount.has_value)
            return mount_info.mount.error_code;

        return mount_info.mount.value->info(mount_info.new_path);
    }

    optional<uint32_t> File::read(void*, uint32_t) {
        return error_t::ENOSYS;
    }

    optional<uint32_t> File::write(void*, uint32_t) {
        return error_t::ENOSYS;
    }

    optional<int64_t> File::seek(int64_t, seek_mode) {
        return error_t::ENOSYS;
    }

    optional<dir_entry> File::readdir() {
        return error_t::ENOTDIR;
    }

    void File::close() {}
}