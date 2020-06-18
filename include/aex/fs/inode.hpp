#pragma once

#include "aex/fs/directory.hpp"
#include "aex/fs/fs.hpp"
#include "aex/mem/smartptr.hpp"
#include "aex/optional.hpp"

#include <stdint.h>

namespace AEX::FS {
    class ControlBlock;

    class INode {
        public:
        int id;

        int device_id = -1;

        fs_type_t type = fs_type_t::UNKNOWN;

        uint64_t block_count;
        uint64_t size;

        ControlBlock* control_block;

        virtual ~INode();

        virtual error_t readBlocks(void* buffer, uint64_t block, uint16_t count);
        virtual error_t writeBlocks(const void* buffer, uint64_t block, uint16_t count);

        virtual error_t update();

        virtual optional<dir_entry> readDir(dir_context* ctx);

        bool is_regular() {
            return (type & fs_type_t::REGULAR) == fs_type_t::REGULAR;
        }

        bool is_directory() {
            return (type & fs_type_t::DIRECTORY) == fs_type_t::DIRECTORY;
        }

        bool is_block() {
            return (type & fs_type_t::BLOCK) == fs_type_t::BLOCK;
        }

        private:
    };

    typedef Mem::SmartPointer<INode> INode_SP;
}