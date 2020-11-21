#pragma once

#include "aex/fs/directory.hpp"
#include "aex/fs/type.hpp"
#include "aex/mem/smartptr.hpp"
#include "aex/optional.hpp"
#include "aex/types.hpp"

namespace AEX::FS {
    class ControlBlock;

    class INode {
        public:
        int id;

        int device_id = -1;

        fs_type_t type = FT_UNKNOWN;

        uint64_t block_count;
        uint64_t size;

        ControlBlock* control_block;

        virtual ~INode();

        virtual error_t readBlocks(void* buffer, uint64_t start, uint16_t count);
        virtual error_t writeBlocks(const void* buffer, uint64_t start, uint16_t count);

        virtual error_t update();

        virtual optional<dir_entry> readDir(dir_context* ctx);

        bool is_regular() {
            return (type & FT_REGULAR) == FT_REGULAR;
        }

        bool is_directory() {
            return (type & FT_DIRECTORY) == FT_DIRECTORY;
        }

        bool is_block() {
            return (type & FT_BLOCK) == FT_BLOCK;
        }

        private:
    };

    typedef Mem::SmartPointer<INode> INode_SP;
}