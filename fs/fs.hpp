#pragma once

#include "aex/fs/mount.hpp"
#include "aex/mem.hpp"
#include "aex/optional.hpp"

namespace AEX::FS {
    struct mount_info {
        optional<Mem::SmartPointer<Mount>> mount;

        const char* new_path;

        mount_info(optional<Mem::SmartPointer<Mount>> mount, const char* new_path) {
            this->mount    = mount;
            this->new_path = new_path;
        }
    };

    void init();

    mount_info find_mount(const char* path);
}