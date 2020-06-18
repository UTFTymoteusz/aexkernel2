#pragma once

#include "aex/fs/directory.hpp"
#include "aex/mem/smartptr.hpp"
#include "aex/optional.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX::FS {
    class INode;

    typedef Mem::SmartPointer<INode> INode_SP;

    class ControlBlock {
        public:
        char label[64];

        int root_inode_id;

        virtual optional<INode_SP> getINode(INode_SP dir, int id);

        virtual error_t readINodeBlocks(INode_SP inode, uint8_t* buffer, uint64_t block,
                                        uint16_t count);
        virtual error_t writeINodeBlocks(INode_SP inode, const uint8_t* buffer, uint64_t block,
                                         uint16_t count);

        virtual error_t updateINode(INode_SP inode);


        virtual optional<dir_entry> readINodeDir(INode_SP inode, dir_context* ctx);

        private:
    };
}