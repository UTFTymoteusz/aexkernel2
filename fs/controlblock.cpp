#include "aex/fs/controlblock.hpp"

#include "aex/errno.hpp"
#include "aex/fs/inode.hpp"
#include "aex/printk.hpp"
#include "aex/string.hpp"

namespace AEX::FS {
    ControlBlock::~ControlBlock() {}

    optional<INode_SP> ControlBlock::get(INode_SP, dirent, ino_t) {
        return ENOSYS;
    }

    optional<INode_SP> ControlBlock::find(const char* lpath) {
        auto inode = ENSURE_OPT(get(INode_SP::getNull(), dirent(), root_inode_id));

        inode->id            = root_inode_id;
        inode->control_block = this;

        for (auto walker = Walker(lpath); auto piece = walker.next();) {
            auto context = dir_context();
            bool found   = false;

            while (true) {
                auto readd_try = inode->readDir(&context);
                if (!readd_try) {
                    if (readd_try.error == ENONE)
                        break;

                    return readd_try.error;
                }

                if (strcmp(readd_try.value.name, piece) != 0)
                    continue;

                found = true;

                if (!walker.final() && !readd_try.value.is_directory())
                    return ENOENT;

                inode     = ENSURE_OPT(get(inode, readd_try.value, readd_try.value.inode_id));
                inode->id = readd_try.value.inode_id;
                inode->control_block = this;

                break;
            }

            if (!found)
                return ENOENT;
        }

        return inode;
    }

    ino_t ControlBlock::nextINodeID() {
        return Mem::atomic_fetch_add(&m_inocurrent, (ino_t) 1);
    }
}