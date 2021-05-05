#include "table.hpp"

namespace AEX::FS {
    Table::Table(Dev::BlockHandle handle, blksize_t sector_size, off_t start, uint16_t count,
                 uint32_t cluster_count)
        : m_handle(handle) {
        m_sector_size   = sector_size;
        m_start         = start;
        m_count         = count;
        m_cluster_count = cluster_count;
    }

    Table::~Table() {}

    cluster_t Table::next(cluster_t) {
        return 0xFFFFFFFF;
    }
}