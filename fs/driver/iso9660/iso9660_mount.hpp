#pragma once

#include "aex/dev/block.hpp"
#include "aex/errno.hpp"
#include "aex/fs/mount.hpp"
#include "aex/optional.hpp"

#include "types.hpp"

#define BLOCK_SIZE 2048

namespace AEX::FS {
    class ISO9660Mount : public Mount {
      public:
        ISO9660Mount(Mem::SmartPointer<Dev::Block> block, const iso9660_dentry& root_dentry) {
            _root_dentry = root_dentry;
            _block_dev   = block;
        }

        optional<Mem::SmartPointer<File>> opendir(const char* lpath);

        optional<file_info> info(const char* lpath);

      private:
        Mem::SmartPointer<Dev::Block> _block_dev;
        iso9660_dentry                _root_dentry;

        optional<iso9660_dentry> findDentry(const char* lpath);
    };
}