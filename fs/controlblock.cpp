#include "aex/fs/controlblock.hpp"

#include "aex/errno.hpp"
#include "aex/fs/inode.hpp"
#include "aex/printk.hpp"
#include "aex/string.hpp"

namespace AEX::FS {
    ControlBlock::~ControlBlock() {}

    optional<INode_SP> ControlBlock::getINode(INode_SP, dir_entry, int) {
        return ENOSYS;
    }

    optional<INode_SP> ControlBlock::findINode(const char* lpath) {
        auto inode_try = getINode(INode_SP::getNull(), dir_entry(), root_inode_id);
        if (!inode_try)
            return inode_try.error_code;

        auto inode = inode_try.value;

        inode->id            = root_inode_id;
        inode->control_block = this;

        for (auto walker = Walker(lpath); auto piece = walker.next();) {
            auto context = dir_context();
            bool found   = false;

            while (true) {
                auto readd_try = inode->readDir(&context);
                if (!readd_try)
                    return readd_try.error_code;

                if (strcmp(readd_try.value.name, piece) != 0)
                    continue;

                found = true;

                if (!walker.isFinal() && !readd_try.value.is_directory())
                    return ENOENT;

                inode_try = getINode(inode, readd_try.value, readd_try.value.inode_id);
                if (!inode_try)
                    return inode_try.error_code;

                inode                = inode_try.value;
                inode->id            = readd_try.value.inode_id;
                inode->control_block = this;

                break;
            }

            if (!found)
                return ENOENT;
        }

        return inode;
    }
}