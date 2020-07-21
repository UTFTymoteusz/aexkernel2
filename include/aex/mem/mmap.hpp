#pragma once

#include "aex/fs/file.hpp"
#include "aex/optional.hpp"

namespace AEX::Mem {
    class MMapRegion {
        public:
        void*  start;
        size_t len;

        MMapRegion(void* addr, size_t length);
        MMapRegion(void* addr, size_t length, FS::File_SP file, int64_t offset);
        virtual ~MMapRegion();

        virtual error_t read(void* dst, int64_t offset, uint32_t count);

        protected:
        optional<FS::File_SP> _file;
        int64_t               _file_offset;
    };
}