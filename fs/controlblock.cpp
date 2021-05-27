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

    void ControlBlock::unlink(ino_t) {}

    optional<find_result> ControlBlock::find(const char* lpath, bool allow_incomplete) {
        auto parent = ENSURE_OPT(get(INode_SP::null(), dirent(), root_inode_id));
        auto inode  = INode_SP::null();
        int  steps  = 0;

        parent->id            = root_inode_id;
        parent->control_block = this;
        parent->mutex.acquire();

        for (auto walker = Walker(lpath); auto piece = walker.next();) {
            auto context = dir_context();
            bool found   = false;

            while (true) {
                auto readd_try = parent->readdir(&context);
                if (!readd_try) {
                    if (readd_try.error == ENONE)
                        break;

                    parent->mutex.release();
                    return readd_try.error;
                }

                if (strcmp(readd_try.value.name, piece) != 0)
                    continue;

                found = true;

                if (!walker.final() && !readd_try.value.is_directory()) {
                    parent->mutex.release();
                    return ENOENT;
                }

                inode     = ENSURE_OPT(get(parent, readd_try.value, readd_try.value.inode_id));
                inode->id = readd_try.value.inode_id;
                inode->control_block = this;

                if (!walker.final()) {
                    parent->mutex.release();

                    parent = inode;
                    inode  = INode_SP::null();

                    parent->mutex.acquire();
                }

                break;
            }

            if (!found) {
                inode = INode_SP::null();

                if (walker.final() && allow_incomplete)
                    return find_result{inode, parent};

                parent->mutex.release();
                return ENOENT;
            }

            steps++;
        }

        if (steps == 0) {
            parent->mutex.release();

            inode  = parent;
            parent = INode_SP::null();
        }

        return find_result{inode, parent};
    }

    ino_t ControlBlock::nextINodeID() {
        return Mem::atomic_fetch_add(&m_inocurrent, (ino_t) 1);
    }
}