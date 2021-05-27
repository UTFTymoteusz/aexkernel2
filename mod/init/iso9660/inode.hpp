#pragma once

#include "aex/dev.hpp"
#include "aex/fs/inode.hpp"
#include "aex/printk.hpp"

#include "controlblock.hpp"
#include "types.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX::FS {
    class ISO9600DirectoryINode : public INode {
        public:
        ISO9600DirectoryINode(iso9660_dentry dentry) {
            m_dentry = dentry;

            size       = dentry.data_len.le;
            type       = FT_DIRECTORY;
            block_size = BLOCK_SIZE;

            hard_links = 1;
        }

        optional<dirent> readdir(dir_context* ctx) {
            if (ctx->pos >= size)
                return {};

            char name_buffer[NAME_MAX];
            name_buffer[NAME_MAX - 1] = '\0';

            uint8_t buffer[256];

            while (ctx->pos < size) {
                ((ISO9660ControlBlock*) control_block)
                    ->block_handle.read(buffer, m_dentry.data_lba.le * BLOCK_SIZE + ctx->pos,
                                        sizeof(buffer));

                auto ldentry = (iso9660_dentry*) buffer;

                uint8_t len = ldentry->len;
                if (len == 0) {
                    ctx->pos = int_floor<int64_t>(ctx->pos + BLOCK_SIZE, BLOCK_SIZE);
                    if (ctx->pos >= size)
                        return {};

                    continue;
                }

                ctx->pos += len;

                if (ldentry->name_len == 1 &&
                    (ldentry->name[0] == '\0' || ldentry->name[0] == '\1'))
                    continue;

                memcpy(name_buffer, ldentry->name,
                       min<size_t>(sizeof(name_buffer) - 1, ldentry->name_len));
                name_buffer[ldentry->name_len] = '\0';

                clean_name(name_buffer);

                // I'm too lazy atm
                int lpos = sizeof(iso9660_dentry) + ldentry->name_len;
                lpos += lpos % 2;

                int max_len = 255 - lpos;

                while (lpos < max_len) {
                    int susp_pos = lpos;

                    if (buffer[susp_pos] < 'A' || buffer[susp_pos] > 'Z')
                        break;

                    lpos += buffer[susp_pos + 2];

                    if (memcmp(&buffer[susp_pos], "CE", 2) == 0)
                        kpanic("iso9660: CE encountered");
                    else if (memcmp(&buffer[susp_pos], "NM", 2) == 0) {
                        strncpy(name_buffer, (const char*) &buffer[susp_pos + 5],
                                min(buffer[susp_pos + 2] - 5 + 1, 255));
                    }
                }

                auto dentry_ret = dirent(name_buffer, ctx->pos - len, ldentry->data_lba.le);

                dentry_ret.type = ldentry->isDirectory() ? FT_DIRECTORY : FT_REGULAR;

                return dentry_ret;
            }

            return {};
        }

        // TODO: checks
        error_t seekdir(dir_context* ctx, long pos) {
            if (ctx->pos >= size)
                return ERANGE;

            ctx->pos = pos;
            return ENONE;
        }

        long telldir(dir_context* ctx) {
            return ctx->pos;
        }

        private:
        iso9660_dentry m_dentry;

        void clean_name(char* buffer) {
            for (int i = 0; i < NAME_MAX; i++) {
                if (buffer[i] == ';')
                    buffer[i] = '\0';
            }
        }

        friend class ISO9660ControlBlock;
    };

    class ISO9600FileINode : public INode {
        public:
        ISO9600FileINode(iso9660_dentry dentry) {
            m_dentry = dentry;

            size       = dentry.data_len.le;
            type       = FT_REGULAR;
            block_size = BLOCK_SIZE;

            hard_links = 1;
        }

        error_t read(void* buffer, blk_t block, blkcnt_t count) {
            ((ISO9660ControlBlock*) control_block)
                ->block_handle.read(buffer, m_dentry.data_lba.le * block_size + block * block_size,
                                    count * block_size);

            return ENONE;
        }

        error_t write(const void*, uint64_t, blkcnt_t) {
            return EROFS;
        }

        private:
        iso9660_dentry m_dentry;

        friend class ISO9660ControlBlock;
    };
}