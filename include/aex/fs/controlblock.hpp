#pragma once

#include "aex/dev/blockhandle.hpp"
#include "aex/fs/directory.hpp"
#include "aex/fs/types.hpp"
#include "aex/mem/cache.hpp"
#include "aex/mem/smartptr.hpp"
#include "aex/mutex.hpp"
#include "aex/optional.hpp"
#include "aex/utility.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX::FS {
    class API ControlBlock {
        public:
        char      label[64];
        ino_t     root_inode_id;
        blksize_t block_size;

        virtual ~ControlBlock();

        virtual optional<INode_SP> get(INode_SP dir, dirent dentry, ino_t id);
        optional<INode_SP>         find(const char* lpath);

        protected:
        Mutex                m_mutex;
        Mem::Cache<INode_SP> m_cache;
        ino_t                m_inocurrent = 1;

        ino_t nextINodeID();
    };
}