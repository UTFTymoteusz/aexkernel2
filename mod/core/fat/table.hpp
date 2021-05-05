#pragma once

#include "aex/dev/blockhandle.hpp"
#include "aex/mem/cache.hpp"

#include "types.hpp"

namespace AEX::FS {
    class Table {
        public:
        Table(Dev::BlockHandle handle, blksize_t sector_size, off_t start, uint16_t count,
              uint32_t cluster_count);
        virtual ~Table();

        virtual cluster_t next(cluster_t cluster);

        protected:
        Mem::Cache<uint8_t*> m_cache;
        Dev::BlockHandle     m_handle;

        blksize_t m_sector_size;
        off_t     m_start;
        uint16_t  m_count;
        uint32_t  m_cluster_count;
    };
}