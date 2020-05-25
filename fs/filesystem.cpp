#include "aex/fs/filesystem.hpp"

#include "aex/fs/fs.hpp"
#include "aex/printk.hpp"
#include "aex/string.hpp"

namespace AEX::FS {
    Filesystem::Filesystem(const char* name) {
        strncpy(this->name, name, sizeof(this->name));

        filesystems.addRef(this);
        printk(PRINTK_OK "fs: Registered filesystem '%s'\n", this->name);
    }

    Filesystem::~Filesystem() {}

    optional<Mount*> Filesystem::mount(const char*) {
        return {};
    }
}