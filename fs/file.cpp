#include "aex/fs/file.hpp"

#include "aex/string.hpp"

#include "fs/fs.hpp"

namespace AEX::FS {
    dir_entry::dir_entry(const char* name) {
        strncpy(this->name, name, sizeof(this->name));
    }

    File::~File() {}

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

    optional<dir_entry> File::readdir() {
        return error_t::ENOTDIR;
    }
}