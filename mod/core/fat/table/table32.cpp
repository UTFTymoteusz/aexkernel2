#include "table32.hpp"

#include "aex/printk.hpp"

namespace AEX::FS {
    Table32::Table32(Dev::BlockHandle handle, blksize_t sector_size, off_t start, uint16_t count,
                     uint32_t cluster_count)
        : Table(handle, sector_size, start, count, cluster_count) {}

    Table32::~Table32() {}

    cluster_t Table32::next(cluster_t cluster) {
        little_endian<cluster_t> next;
        m_handle.read(&next, m_start + cluster * sizeof(cluster_t), sizeof(cluster_t));

        cluster = next & 0x0FFFFFFF;
        if (cluster >= 0x0FFFFFF7)
            return 0xFFFFFFFF;

        return cluster;
    }
}