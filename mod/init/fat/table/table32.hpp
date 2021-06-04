#pragma once

#include "../table.hpp"

namespace AEX::FS {
    class Table32 : public Table {
        public:
        Table32(Dev::BlockHandle handle, blksize_t sector_size, off_t start, uint16_t count,
                uint32_t cluster_count);
        ~Table32();

        cluster_t next(cluster_t cluster);
        void      link(cluster_t cluster, cluster_t next);
        cluster_t find();

        private:
        cluster_t _next(cluster_t cluster);
    };
}