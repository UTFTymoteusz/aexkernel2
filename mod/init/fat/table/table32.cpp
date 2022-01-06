#include "table32.hpp"

#include "aex/printk.hpp"

namespace AEX::FS {
    Table32::Table32(Dev::BlockHandle handle, blksize_t sector_size, off_t start, uint16_t count,
                     uint32_t cluster_count)
        : Table(handle, sector_size, start, count, cluster_count) {}

    Table32::~Table32() {
        SCOPE(mutex);
    }

    cluster_t Table32::next(cluster_t cluster) {
        AEX_ASSERT(cluster <= m_cluster_count);
        AEX_ASSERT(!mutex.tryAcquire());

        cluster_t next = _next(cluster) & 0x0FFFFFFF;
        if (next >= 0x0FFFFFF7)
            next = 0xFFFFFFFF;

        return next;
    }

    void Table32::link(cluster_t cluster, cluster_t next) {
        AEX_ASSERT(cluster <= m_cluster_count);
        AEX_ASSERT(!mutex.tryAcquire());

        next |= _next(cluster) & 0xF0000000;

        cluster_t next_l = from_le(next);
        m_handle.write(&next_l, m_start + cluster * sizeof(cluster_t), sizeof(cluster_t));
    }

    cluster_t Table32::find() {
        AEX_ASSERT(!mutex.tryAcquire());

        for (cluster_t i = 0; i < m_cluster_count; i++) {
            if ((_next(i) & 0x0FFFFFFF) == 0x00000000)
                return i;
        }

        return 0xFFFFFFFF;
    }

    cluster_t Table32::_next(cluster_t cluster) {
        le<cluster_t> next;
        m_handle.read(&next, m_start + cluster * sizeof(cluster_t), sizeof(cluster_t));

        cluster = next;
        return cluster;
    }
}