#include "inodefile.hpp"

#include "aex/fs/controlblock.hpp"
#include "aex/fs/file.hpp"
#include "aex/fs/inode.hpp"
#include "aex/math.hpp"
#include "aex/mem.hpp"
#include "aex/proc.hpp"
#include "aex/types.hpp"

namespace AEX::FS {
    INodeFile::INodeFile(INode_SP inode, int mode) {
        m_inode = inode;
        m_mode  = mode;

        SCOPE(m_inode->mutex);

        AEX_ASSERT(m_inode->block_size);
        m_inode->opened++;
    }

    INodeFile::~INodeFile() {
        SCOPE(m_inode->mutex);

        auto entry = m_inode->getCacheEntry(this);
        if (entry) {
            m_inode->writeCheck(entry);
            m_inode->evictCacheEntry(this);
        }

        if (m_inode->truncate_cached) {
            m_inode->truncate(m_inode->size);
            m_inode->truncate_cached = false;
        }

        m_inode->opened--;

        printk("fs: Closed inode %i (%i, %i)\n", m_inode->id, m_inode->opened, m_inode->hard_links);

        if (!m_inode->opened && m_inode->hard_links == 0)
            m_inode->purge();
    }

    optional<ssize_t> INodeFile::read(void* buffer, size_t count) {
        if (!(m_mode & O_RDONLY))
            return EBADF;

        SCOPE(m_inode->mutex);

        if (m_pos >= m_inode->size)
            return 0;

        count = min<size_t>(m_inode->size - m_pos, count);
        if (count == 0)
            return 0;

        error_t error = readBlocks(buffer, m_pos, count);
        if (error)
            return error;

        m_pos += count;
        return count;
    }

    optional<ssize_t> INodeFile::write(const void* buffer, size_t count) {
        if (!(m_mode & O_RDONLY))
            return EBADF;

        if (count == 0)
            return 0;

        SCOPE(m_inode->mutex);

        if (m_mode & O_APPEND)
            m_pos = m_inode->size;

        if (m_pos + count > (size_t) m_inode->size) {
            error_t error            = m_inode->truncate(m_pos + count, true);
            m_inode->truncate_cached = true;
            if (error)
                return error;
        }

        error_t error = writeBlocks(buffer, m_pos, count);
        if (error)
            return error;

        m_pos += count;
        return count;
    }

    optional<off_t> INodeFile::seek(off_t offset, seek_mode mode) {
        SCOPE(m_inode->mutex);

        off_t new_pos = 0;

        switch (mode) {
        case SEEK_SET:
            new_pos = offset;
            break;
        case SEEK_CURRENT:
            new_pos = m_pos + offset;
            break;
        case SEEK_END:
            new_pos = m_inode->size + offset;
            break;
        default:
            break;
        }

        if (new_pos < 0 || (off_t) new_pos > m_inode->size)
            return EINVAL;

        m_pos = new_pos;
        return new_pos;
    }

    error_t INodeFile::close() {
        SCOPE(m_inode->mutex);
        return ENONE;
    }

    optional<file_info> INodeFile::finfo() {
        SCOPE(m_inode->mutex);

        file_info finfo;

        finfo.containing_dev_id = 0;
        finfo.inode             = m_inode->id;
        finfo.type              = m_inode->type;
        finfo.mode              = m_inode->mode;
        finfo.hard_links        = m_inode->hard_links;

        finfo.uid = m_inode->uid;
        finfo.gid = m_inode->gid;
        finfo.dev = m_inode->dev;

        finfo.access_time = m_inode->access_time;
        finfo.modify_time = m_inode->modify_time;
        finfo.change_time = m_inode->change_time;

        finfo.blocks     = m_inode->block_count;
        finfo.block_size = m_inode->block_size;
        finfo.total_size = m_inode->size;

        return finfo;
    }

    optional<Mem::Region*> INodeFile::mmap(Proc::Process* process, void*, size_t len, int flags,
                                           FS::File_SP file, FS::off_t offset) {
        SCOPE(m_inode->mutex);

        void* alloc_addr = process->pagemap->map(len, 0, PAGE_ARBITRARY | flags);
        return new Mem::FileBackedRegion(process->pagemap, alloc_addr, len, file, offset);
    }

    error_t INodeFile::readBlocks(void* buffer, off_t start, size_t len) {
        uint8_t* bytes       = (uint8_t*) buffer;
        blk_t    combo_start = 0;
        blkcnt_t combo_count = 0;

        while (len > 0) {
            off_t  loffset = start - int_floor<uint64_t>(start, m_inode->block_size);
            size_t llen    = min<size_t>(m_inode->block_size - loffset, len);
            auto   entry   = m_inode->getCacheEntry(start / m_inode->block_size);

            if (!isPerfect(start, llen) || entry) {
                if (entry) {
                    memcpy(bytes + combo_count * m_inode->block_size, entry->data + loffset, llen);
                }

                if (combo_count != 0) {
                    auto error = m_inode->read(bytes, combo_start, combo_count);
                    if (error)
                        return error;

                    bytes += combo_count * m_inode->block_size;
                    combo_count = 0;
                }

                entry = m_inode->getCacheEntry(this);
                m_inode->writeCheck(entry);

                entry->id = start / m_inode->block_size;

                auto error = m_inode->read(entry->data, start / m_inode->block_size, 1);
                if (error) {
                    entry->id = 0xFFFFFFFFFFFFFFFF;
                    return error;
                }

                memcpy(bytes, entry->data + loffset, llen);
                bytes += llen;
            }
            else {
                if (combo_count == 0)
                    combo_start = start / m_inode->block_size;

                combo_count++;
            }

            start += llen;
            len -= llen;
        }

        if (combo_count) {
            auto error = m_inode->read(bytes, combo_start, combo_count);
            if (error)
                return error;

            auto entry = m_inode->getCacheEntry(this);
            m_inode->writeCheck(entry);

            entry->id = combo_start + combo_count - 1;

            memcpy(entry->data, bytes + (combo_count - 1) * m_inode->block_size,
                   m_inode->block_size);
        }

        return ENONE;
    }

    error_t INodeFile::writeBlocks(const void* buffer, off_t start, size_t len) {
        const uint8_t* bytes       = (const uint8_t*) buffer;
        blk_t          combo_start = 0;
        blkcnt_t       combo_count = 0;

        while (len > 0) {
            off_t  loffset = start - int_floor<uint64_t>(start, m_inode->block_size);
            size_t llen    = min<size_t>(m_inode->block_size - loffset, len);
            auto   entry   = m_inode->getCacheEntry(start / m_inode->block_size);

            if (!isPerfect(start, llen) || entry) {
                if (entry) {
                    memcpy(entry->data + loffset, bytes + combo_count * m_inode->block_size, llen);
                    entry->changed = true;
                }

                if (combo_count != 0) {
                    auto error = m_inode->write(bytes, combo_start, combo_count);
                    if (error)
                        return error;

                    bytes += combo_count * m_inode->block_size;
                    combo_count = 0;
                }

                if (!entry) {
                    entry = m_inode->getCacheEntry(this);
                    m_inode->writeCheck(entry);

                    entry->id = start / m_inode->block_size;
                    m_inode->read(entry->data, start / m_inode->block_size, 1);

                    memcpy(entry->data + loffset, bytes, llen);
                    entry->changed = true;
                }
            }
            else {
                if (combo_count == 0)
                    combo_start = start / m_inode->block_size;

                combo_count++;
            }

            start += llen;
            len -= llen;
        }

        if (combo_count) {
            auto error = m_inode->write(bytes, combo_start, combo_count);
            if (error)
                return error;
        }

        return ENONE;
    }

    bool INodeFile::isPerfect(off_t start, size_t len) {
        if (start % m_inode->block_size != 0)
            return false;

        if (len % m_inode->block_size != 0)
            return false;

        return true;
    }
}