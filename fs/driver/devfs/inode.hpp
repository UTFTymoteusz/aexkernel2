#pragma once

#include "aex/dev.hpp"
#include "aex/fs/inode.hpp"
#include "aex/printk.hpp"

namespace AEX::FS {
    class DevFSRootINode : public INode {
        public:
        DevFSRootINode() {
            type = FT_DIRECTORY;
        }

        optional<dirent> readDir(dir_context* ctx) {
            while ((int) ctx->pos < Dev::devices.count()) {
                auto device = Dev::devices.get(ctx->pos);
                if (!device)
                    continue;

                ctx->pos++;

                auto dentry = dirent(device->name, ctx->pos - 1, ctx->pos + 1);

                switch (device->type) {
                case Dev::DEV_BLOCK:
                    dentry.type = FT_BLOCK;
                    break;
                case Dev::DEV_CHAR:
                    dentry.type = FT_CHAR;
                    break;
                case Dev::DEV_NET:
                    dentry.type = FT_NET;
                    break;
                }

                return dentry;
            }

            return {};
        }
    };
}