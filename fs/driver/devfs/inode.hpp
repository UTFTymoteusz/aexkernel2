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

        optional<dir_entry> readDir(dir_context* ctx) {
            while ((int) ctx->pos < Dev::devices.count()) {
                auto device = Dev::devices.get(ctx->pos);
                if (!device)
                    continue;

                ctx->pos++;

                return dir_entry(device->name, ctx->pos - 1, ctx->pos + 1);
            }

            return {};
        }
    };
}