#include "aex/fs/inode.hpp"

namespace AEX::FS {
    INode::cache_entry* INode::getCacheEntry(File* file) {
        for (int i = 0; i < m_cache.count(); i++)
            if (m_cache[i]->file == file)
                return m_cache[i];

        auto entry = new cache_entry();

        entry->file = file;
        entry->id   = 0xFFFFFFFFFFFFFFFF;
        entry->data = new uint8_t[block_size];

        m_cache.push(entry);

        return entry;
    }

    INode::cache_entry* INode::getCacheEntry(blk_t id) {
        for (int i = 0; i < m_cache.count(); i++)
            if (m_cache[i]->id == id)
                return m_cache[i];

        return nullptr;
    }

    void INode::evictCacheEntry(File* file) {
        for (int i = 0; i < m_cache.count(); i++) {
            if (m_cache[i]->file != file)
                continue;

            delete m_cache[i]->data;
            delete m_cache[i];

            m_cache.erase(i);
            return;
        }
    }
}
