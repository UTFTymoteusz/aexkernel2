#include "iso9660_mount.hpp"

#include "aex/fs/path.hpp"
#include "aex/math.hpp"
#include "aex/string.hpp"

#include "types.hpp"

namespace AEX::FS {
    optional<file_info> ISO9660Mount::info(const char* lpath) {
        printk("iso9660: info(%s)\n", lpath);

        auto dentry_try = findDentry(lpath);
        if (!dentry_try.has_value)
            return dentry_try.error_code;

        auto dentry = dentry_try.value;
        auto info   = file_info();

        info.type              = dentry.isDirectory() ? type_t::DIRECTORY : type_t::REGULAR;
        info.total_size        = dentry.data_len.le;
        info.containing_dev_id = _block_dev->id;

        return info;
    }

    optional<iso9660_dentry> ISO9660Mount::findDentry(const char* lpath) {
        iso9660_dentry dentry = _root_dentry;

        uint8_t buffer[BLOCK_SIZE];
        char    name_buffer[Path::MAX_FILENAME_LEN];

        name_buffer[Path::MAX_FILENAME_LEN - 1] = '\0';

        // this code is a mess, help
        for (auto walker = Path::Walker(lpath); auto piece = walker.next();) {
            _block_dev->read(buffer, dentry.data_lba.le * BLOCK_SIZE, BLOCK_SIZE);

            bool found = false;

            for (uint32_t pos = 0; pos < dentry.data_len.le;) {
                auto floored = (pos / BLOCK_SIZE) * BLOCK_SIZE;
                auto ldentry = (iso9660_dentry*) &buffer[pos - floored];

                uint8_t len = ldentry->len;
                if (len == 0) {
                    pos = ((pos + BLOCK_SIZE) / BLOCK_SIZE) * BLOCK_SIZE;
                    continue;
                }

                pos += len;

                if (ldentry->name_len == 1 &&
                    (ldentry->name[0] == '\0' || ldentry->name[0] == '\1'))
                    continue;

                memcpy(name_buffer, ldentry->name, min(sizeof(name_buffer) - 1, ldentry->name_len));
                name_buffer[ldentry->name_len] = '\0';

                cleanupName(name_buffer);

                if (strcmp(name_buffer, piece) == 0) {
                    if (walker.isFinal()) {
                        if (ldentry->isDirectory())
                            return error_t::EISDIR;
                    }
                    else {
                        if (!ldentry->isDirectory())
                            return error_t::ENOTDIR;
                    }

                    dentry = *ldentry;
                    found  = true;

                    break;
                }
            }

            if (!found)
                return error_t::ENOENT;
        }

        return dentry;
    }

    void ISO9660Mount::cleanupName(char* buffer) {
        for (int i = 0; i < Path::MAX_FILENAME_LEN; i++) {
            if (buffer[i] == ';')
                buffer[i] = '\0';
        }
    }
}