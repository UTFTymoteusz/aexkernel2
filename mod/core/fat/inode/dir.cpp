#include "../inode.hpp"

namespace AEX::FS {
    optional<dirent> FATDirectoryINode::readDir(dir_context* ctx) {
        if (ctx->pos >= size)
            return {};

        auto           fat_block = (FATControlBlock*) control_block;
        fat_dirent     fat_dirent;
        fat_dirent_lfn longs[16];
        int            long_count = 0;

        char filename[MAX_FILENAME_LEN];
        int  filename_offset;

        while (ctx->pos < size) {
            cluster_t lcluster = ctx->pos / fat_block->m_cluster_size;
            cluster_t pcluster = m_chain.at(lcluster);
            off_t     offset   = ctx->pos - (lcluster * fat_block->m_cluster_size);

            ctx->pos += sizeof(fat_dirent);

            fat_block->block_handle.read(&fat_dirent, fat_block->getOffset(pcluster, offset),
                                         sizeof(fat_dirent));
            if (fat_dirent.attributes == FAT_LFN) {
                fat_dirent_lfn& lfn = (fat_dirent_lfn&) fat_dirent;

                int order = lfn.order - 1;
                if (order < 0) {
                    printk(PRINTK_WARN "something's up with the lfn entry\n");
                    continue;
                }

                long_count++;
                longs[order] = lfn;

                continue;
            }

            if (fat_dirent.filename[0] == '\0')
                break;

            filename_offset = 0;

            if (long_count) {
                for (int i = 0; i < long_count; i++) {
                    int len = readLFN(longs[i], filename + filename_offset,
                                      sizeof(filename) - filename_offset);

                    filename_offset += len;
                }
            }
            else {
                strncpy(filename, fat_dirent.filename, 12);
                if (strcmp(filename, ".          ") == 0 || strcmp(filename, "..         ") == 0)
                    continue;

                NOT_IMPLEMENTED;
            }

            filename[filename_offset] = '\0';
            long_count                = 0;

            auto   assoc    = getAssoc(fat_dirent.filename);
            ino_t  inode_id = assoc ? assoc.value : createAssoc(fat_dirent);
            dirent aex_dirent;

            strncpy(aex_dirent.name, filename, sizeof(aex_dirent.name));
            aex_dirent.type     = fat_dirent.attributes & FAT_DIRECTORY ? FT_DIRECTORY : FT_REGULAR;
            aex_dirent.inode_id = inode_id;
            aex_dirent.pos      = ctx->pos;

            return aex_dirent;
        }

        return {};
    }

    int FATDirectoryINode::readLFN(fat_dirent_lfn& lfn, char* buffer, int remaining) {
        char charbuffer[8];
        int  count    = 0;
        auto readChar = [&charbuffer, &remaining, &count, &buffer](uint8_t* ptr) {
            strncpy(charbuffer, (char*) ptr, sizeof(charbuffer));
            if (charbuffer[0] == '\0') {
                remaining = 0;
                return;
            }

            if (strlen(charbuffer) > 1) {
                charbuffer[0] = '?';
                charbuffer[1] = '\0';
            }

            int len = strlen(charbuffer);
            if (len < 0 || len > remaining)
                return;

            buffer[count] = charbuffer[0];

            count += len;
            remaining -= len;
        };

        remaining--;

        for (int i = 0; i < 5; i++)
            readChar(&lfn.chars0[i * 2]);

        for (int i = 0; i < 6; i++)
            readChar(&lfn.chars1[i * 2]);

        for (int i = 0; i < 2; i++)
            readChar(&lfn.chars2[i * 2]);

        return count;
    }

    optional<ino_t> FATDirectoryINode::getAssoc(const char* filename) {
        for (int i = 0; i < m_assocs.count(); i++)
            if (memcmp(filename, m_assocs[i].filename, 11) == 0)
                return m_assocs[i].id;

        return {};
    }

    void FATDirectoryINode::pushAssoc(const char* filename, ino_t id) {
        inode_assoc assoc;

        strncpy(assoc.filename, filename, 12);
        assoc.id = id;

        m_assocs.push(assoc);
    }

    ino_t FATDirectoryINode::createAssoc(fat_dirent dirent) {
        auto fat_block = (FATControlBlock*) control_block;
        SCOPE(fat_block->m_mutex);

        uint32_t first_cluster = dirent.first_cluster_lo | (dirent.first_cluster_hi << 16);
        ino_t    inode_id      = fat_block->nextINodeID();
        INode_SP inode;

        if (dirent.attributes & FAT_DIRECTORY) {
            auto dir = new FATDirectoryINode();

            if (first_cluster) {
                fat_block->fillChain(first_cluster, dir->chain());
                dir->m_filled = true;
            }

            dir->size        = dir->chain().count() * fat_block->block_size;
            dir->block_count = dir->chain().count();
            dir->block_size  = fat_block->block_size;

            inode = INode_SP(dir);
        }
        else {
            auto file = new FATFileINode();

            if (first_cluster) {
                file->chain().first(first_cluster);
                file->m_filled = false;
            }

            file->size       = dirent.size;
            file->block_size = fat_block->block_size;

            inode = INode_SP(file);
        }

        fat_block->m_cache.set(inode_id, inode);
        pushAssoc(dirent.filename, inode_id);

        return inode_id;
    }
}