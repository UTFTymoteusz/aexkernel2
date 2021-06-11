#include "../fat.hpp"
#include "../inode.hpp"

namespace AEX::FS {
    optional<INode_SP> FATDirectory::creat(const char* filename, mode_t, fs_type_t type) {
        AEX_ASSERT(!mutex.tryAcquire());

        if (type != FT_REGULAR && type != FT_DIRECTORY)
            return ENOTSUP;

        int  lfn_count = (strlen(filename) + 1) / 13 + 1;
        int  pos       = 0;
        auto fat_block = (FATControlBlock*) controlblock;

        char shortname[12] = {};
        filename2shortname(shortname, filename);

        if (strlen(filename) > 8)
            increment(shortname);

        while (pos < size) {
            auto fat_dirent = read(pos);
            pos += sizeof(fat_dirent);

            if (fat_dirent.filename[0] == '\x05')
                fat_dirent.filename[0] = '\xE5';

            if (memcmp(shortname, fat_dirent.filename, 11) == 0) {
                increment(shortname);
                pos = 0;
                continue;
            }
        }

        fat_dirent entry = {};
        cluster_t  start = 0;

        if (type == FT_DIRECTORY) {
            SCOPE(fat_block->m_table->mutex);

            start = fat_block->m_table->find();
            if (start == 0xFFFFFFFF)
                return ENOSPC;

            cluster_t parent = m_chain.first();
            if (parent == fat_block->m_root_cluster)
                parent = 0;

            fat_block->m_table->link(start, 0xFFFFFFF8);
            prepare(start, parent);
        }

        memcpy(entry.filename, shortname, 11);
        entry.attributes       = type == FT_DIRECTORY ? FAT_DIRECTORY : 0x00;
        entry.first_cluster_lo = start & 0xFFFF;
        entry.first_cluster_hi = (start >> 16) & 0xFFFF;

        pos = ENSURE_OPT(space(lfn_count + 1));
        write(pos, entry, filename);

        ino_t    id = createAssoc(entry);
        INode_SP inode;

        using(fat_block->m_mutex) {
            inode     = fat_block->m_cache.get(id);
            inode->id = id;

            ((FATINode*) inode.get())->m_parent = fat_block->m_cache.get(this->id).value;
        }

        return inode;
    }

    optional<dirent> FATDirectory::readdir(dir_context* ctx) {
        AEX_ASSERT(!mutex.tryAcquire());

        if (ctx->pos >= size)
            return {};

        fat_dirent_lfn longs[32];
        int            long_count = 0;

        char filename[NAME_MAX + 1];
        int  filename_offset;

        while (ctx->pos < size) {
            auto fat_dirent = read(ctx->pos);
            ctx->pos += sizeof(fat_dirent);

            if (fat_dirent.filename[0] == '\0' || fat_dirent.filename[0] == '\xE5')
                continue;

            if (fat_dirent.attributes == FAT_LFN) {
                fat_dirent_lfn& lfn = (fat_dirent_lfn&) fat_dirent;

                int order = lfn.order - 1;
                if (order < 0) {
                    printk(WARN "something's up with the lfn entry\n");
                    continue;
                }

                long_count++;
                longs[order] = lfn;

                continue;
            }

            if (fat_dirent.filename[0] == '\x05')
                fat_dirent.filename[0] = '\xE5';

            filename_offset = 0;

            if (long_count) {
                for (int i = 0; i < long_count; i++) {
                    int len = readLFN(longs[i], filename + filename_offset,
                                      sizeof(filename) - filename_offset);

                    filename_offset += len;
                }
            }
            else {
                strlcpy(filename, fat_dirent.filename, 12);
                if (strcmp(filename, ".          ") == 0 || strcmp(filename, "..         ") == 0)
                    continue;

                shortname2filename(filename, fat_dirent.filename);
                filename_offset = 12;
            }

            filename[filename_offset] = '\0';
            long_count                = 0;

            auto   assoc    = getAssoc(fat_dirent.filename);
            ino_t  inode_id = assoc ? assoc.value : createAssoc(fat_dirent);
            dirent aex_dirent;

            strlcpy(aex_dirent.name, filename, sizeof(aex_dirent.name));
            aex_dirent.type     = fat_dirent.attributes & FAT_DIRECTORY ? FT_DIRECTORY : FT_REGULAR;
            aex_dirent.inode_id = inode_id;
            aex_dirent.pos      = ctx->pos;

            return aex_dirent;
        }

        return {};
    }

    error_t FATDirectory::link(const char* filename, INode_SP inode) {
        AEX_ASSERT(!mutex.tryAcquire());

        int  lfn_count = (strlen(filename) + 1) / 13 + 1;
        int  pos       = 0;
        auto fat_block = (FATControlBlock*) controlblock;

        char shortname[12] = {};
        filename2shortname(shortname, filename);

        if (strlen(filename) > 8)
            increment(shortname);

        while (pos < size) {
            auto fat_dirent = read(pos);
            pos += sizeof(fat_dirent);

            if (fat_dirent.filename[0] == '\x05')
                fat_dirent.filename[0] = '\xE5';

            if (memcmp(shortname, fat_dirent.filename, 11) == 0) {
                increment(shortname);
                pos = 0;
                continue;
            }
        }

        pushAssoc(shortname, inode->id);

        using(fat_block->m_mutex) {
            using(inode->mutex) {
                ((FATINode*) inode.get())->m_parent = fat_block->m_cache.get(this->id).value;

                fat_dirent entry = {};
                cluster_t  start = ((FATINode*) inode.get())->m_chain.first();

                memcpy(entry.filename, shortname, 11);
                entry.attributes       = inode->type == FT_DIRECTORY ? FAT_DIRECTORY : 0x00;
                entry.first_cluster_lo = start & 0xFFFF;
                entry.first_cluster_hi = (start >> 16) & 0xFFFF;
                entry.size             = inode->size;

                pos = ENSURE_OPT(space(lfn_count + 1));
                write(pos, entry, filename);

                inode->hard_links++;
            }
        }

        return ENONE;
    }

    error_t FATDirectory::unlink(const char* filename) {
        AEX_ASSERT(!mutex.tryAcquire());

        fat_dirent_lfn longs[32];
        int            long_count = 0;

        char filenameB[NAME_MAX + 1];
        int  filenameB_offset;
        int  pos       = 0;
        auto fat_block = (FATControlBlock*) controlblock;

        while (pos < size) {
            auto fat_dirent = read(pos);
            pos += sizeof(fat_dirent);

            if (fat_dirent.filename[0] == '\0' || fat_dirent.filename[0] == '\xE5')
                continue;

            if (fat_dirent.attributes == FAT_LFN) {
                fat_dirent_lfn& lfn = (fat_dirent_lfn&) fat_dirent;

                int order = lfn.order - 1;
                if (order < 0) {
                    printk(WARN "something's up with the lfn entry\n");
                    continue;
                }

                long_count++;
                longs[order] = lfn;

                continue;
            }

            if (fat_dirent.filename[0] == '\x05')
                fat_dirent.filename[0] = '\xE5';

            filenameB_offset = 0;

            if (long_count) {
                for (int i = 0; i < long_count; i++) {
                    int len = readLFN(longs[i], filenameB + filenameB_offset,
                                      sizeof(filenameB) - filenameB_offset);

                    filenameB_offset += len;
                }
            }
            else {
                strlcpy(filenameB, fat_dirent.filename, 12);
                if (strcmp(filenameB, ".          ") == 0 || strcmp(filenameB, "..         ") == 0)
                    continue;

                shortname2filename(filenameB, fat_dirent.filename);
                filenameB_offset = 12;
            }

            filenameB[filenameB_offset] = '\0';

            if (strcmp(filename, filenameB) != 0) {
                long_count = 0;
                continue;
            }

            auto     assoc    = getAssoc(fat_dirent.filename);
            ino_t    inode_id = assoc ? assoc.value : createAssoc(fat_dirent);
            INode_SP inode;

            using(fat_block->m_mutex) {
                inode     = fat_block->m_cache.get(inode_id);
                inode->id = inode_id;

                ((FATINode*) inode.get())->m_parent = fat_block->m_cache.get(this->id).value;

                using(inode->mutex) {
                    inode->hard_links--;

                    printkd(PTKD_FS, "fat: %s: Unlinked inode %i (hard %i, open %i)\n",
                            controlblock->path, inode->id, inode->hard_links, inode->opened);

                    if (inode->is_directory()) {
                        if (inode->opened == 0 && inode->hard_links == 1)
                            inode->purge();
                    }
                    else {
                        if (inode->opened == 0 && inode->hard_links == 0)
                            inode->purge();
                    }
                }
            }

            eraseAssoc(fat_dirent.filename);

            fat_dirent.filename[0] = 0xE5;

            for (int i = 0; i < long_count + 1; i++) {
                pos -= sizeof(fat_dirent);
                write(pos, fat_dirent);
            }

            return ENONE;
        }

        return ENOENT;
    }

    error_t FATDirectory::purge() {
        printkd(PTKD_FS, "fat: %s: Purging inode %i\n", controlblock->path, id);

        auto fat_block = (FATControlBlock*) controlblock;
        SCOPE(fat_block->m_table->mutex);

        for (size_t i = 0; i < m_chain.count(); i++)
            fat_block->m_table->link(m_chain.at(i), 0x00000000);

        return ENONE;
    }

    void FATDirectory::resize(INode* inode, cluster_t first, uint32_t size) {
        auto assoc_try = getAssoc(inode->id);
        if (!assoc_try)
            BROKEN;

        int pos = 0;

        while (pos < this->size) {
            auto fat_dirent = read(pos);
            pos += sizeof(fat_dirent);

            if (fat_dirent.attributes == FAT_LFN)
                continue;

            if (fat_dirent.filename[0] == '\0' || fat_dirent.filename[0] == '\xE5')
                continue;

            if (fat_dirent.filename[0] == '\x05')
                fat_dirent.filename[0] = '\xE5';

            if (memcmp(fat_dirent.filename, assoc_try.value.filename, 11))
                continue;

            printkd(PTKD_FS, "fat: %s: Resizing inode %i (./%s) from %i to %i\n",
                    controlblock->path, inode->id, assoc_try.value.filename, fat_dirent.size, size);

            fat_dirent.size             = size;
            fat_dirent.first_cluster_hi = (first >> 16) & 0xFFFF;
            fat_dirent.first_cluster_lo = first & 0xFFFF;

            write(pos - sizeof(fat_dirent), fat_dirent);
            return;
        }
    }

    int FATDirectory::readLFN(fat_dirent_lfn& lfn, char* buffer, int remaining) {
        char charbuffer[8];
        int  count    = 0;
        auto readChar = [&charbuffer, &remaining, &count, &buffer](uint8_t* ptr) {
            strlcpy(charbuffer, (char*) ptr, sizeof(charbuffer));
            if (charbuffer[0] == '\0') {
                remaining = 0;
                return;
            }

            if (strlen(charbuffer) > 1) {
                charbuffer[0] = '?';
                charbuffer[1] = '\0';
            }

            int len = strlen(charbuffer);
            if (len > remaining)
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

    void FATDirectory::writeLFN(fat_dirent_lfn& lfn, const char* buffer, int remaining) {
        auto writeChar = [&remaining, &buffer](uint8_t* ptr) {
            if (remaining < 0) {
                ptr[0] = '\xFF';
                ptr[1] = '\xFF';

                return;
            }

            ptr[0] = *buffer++;
            ptr[1] = '\0';

            remaining--;
        };

        for (int i = 0; i < 5; i++)
            writeChar(&lfn.chars0[i * 2]);

        for (int i = 0; i < 6; i++)
            writeChar(&lfn.chars1[i * 2]);

        for (int i = 0; i < 2; i++)
            writeChar(&lfn.chars2[i * 2]);
    }

    fat_dirent FATDirectory::read(int pos) {
        auto       fat_block = (FATControlBlock*) controlblock;
        cluster_t  lcluster  = pos / fat_block->m_cluster_size;
        cluster_t  pcluster  = m_chain.at(lcluster);
        off_t      offset    = pos - (lcluster * fat_block->m_cluster_size);
        fat_dirent fat_dirent;

        fat_block->block_handle.read(&fat_dirent, fat_block->getOffset(pcluster, offset),
                                     sizeof(fat_dirent));

        return fat_dirent;
    }

    void FATDirectory::write(int pos, fat_dirent& ent) {
        auto      fat_block = (FATControlBlock*) controlblock;
        cluster_t lcluster  = pos / fat_block->m_cluster_size;
        cluster_t pcluster  = m_chain.at(lcluster);
        off_t     offset    = pos - (lcluster * fat_block->m_cluster_size);

        fat_block->block_handle.write(&ent, fat_block->getOffset(pcluster, offset),
                                      sizeof(fat_dirent));
    }

    void FATDirectory::write(int pos, fat_dirent& ent, const char* filename) {
        int     remaining = strlen(filename);
        uint8_t sum       = lfn_checksum((uint8_t*) ent.filename);
        int     lfn_count = (strlen(filename) + 1) / 13 + 1;

        pos += sizeof(fat_dirent) * lfn_count;

        for (int i = 0; i < lfn_count; i++) {
            pos -= sizeof(fat_dirent);

            fat_dirent      entry = {};
            fat_dirent_lfn& lfn   = (fat_dirent_lfn&) entry;

            writeLFN(lfn, filename + i * 13, remaining);
            remaining -= 13;

            lfn.order     = i + 1;
            lfn.info      = (i == lfn_count - 1) ? 0x2 : 0;
            lfn.checksum  = sum;
            lfn.attribute = FAT_LFN;

            write(pos, entry);
        }

        pos += sizeof(fat_dirent) * lfn_count;

        write(pos, ent);
    }

    optional<int> FATDirectory::space(int rcount) {
        int pos   = 0;
        int start = -1;
        int count = 0;

        while (pos < size) {
            auto fat_dirent = read(pos);

            if (fat_dirent.filename[0] == '\0' || fat_dirent.filename[0] == '\xE5') {
                if (start == -1)
                    start = pos;

                count++;

                if (count > rcount)
                    return start;
            }
            else {
                start = -1;
                count = 0;
            }

            pos += sizeof(fat_dirent);

            if (pos == size) {
                auto err = expand();
                if (err)
                    return err;
            }
        }

        BROKEN;
    }

    error_t FATDirectory::expand() {
        auto      fat_block = (FATControlBlock*) controlblock;
        cluster_t found;

        using(fat_block->m_table->mutex) {
            found = fat_block->m_table->find();
            if (found == 0xFFFFFFFF)
                return ENOSPC;

            fat_block->m_table->link(m_chain.last(), found);
            m_chain.push(found);

            size = m_chain.count() * fat_block->m_cluster_size;

            fat_block->m_table->link(m_chain.last(), 0x0FFFFFF8);
            fat_block->m_table->flush();
        }

        prepare(found);

        return ENONE;
    }

    void FATDirectory::prepare(cluster_t cluster, cluster_t parent) {
        auto       fat_block = (FATControlBlock*) controlblock;
        fat_dirent ent       = {};
        ent.filename[0]      = 0xE5;

        for (int i = 64; i < fat_block->m_cluster_size; i += sizeof(ent))
            fat_block->block_handle.write(&ent, fat_block->getOffset(cluster, i),
                                          fat_block->m_cluster_size);

        memcpy(ent.filename, ".          ", 11);
        ent.attributes       = FAT_DIRECTORY;
        ent.first_cluster_lo = cluster & 0xFFFF;
        ent.first_cluster_hi = (cluster >> 16) & 0xFFFF;

        fat_block->block_handle.write(&ent, fat_block->getOffset(cluster, 0), sizeof(ent));

        memcpy(ent.filename, "..         ", 11);
        ent.attributes       = FAT_DIRECTORY;
        ent.first_cluster_lo = parent & 0xFFFF;
        ent.first_cluster_hi = (parent >> 16) & 0xFFFF;

        fat_block->block_handle.write(&ent, fat_block->getOffset(cluster, 32), sizeof(ent));
    }

    void FATDirectory::prepare(cluster_t cluster) {
        auto       fat_block = (FATControlBlock*) controlblock;
        fat_dirent freeent;
        freeent.filename[0] = 0xE5;

        for (int i = 0; i < fat_block->m_cluster_size; i += sizeof(freeent))
            fat_block->block_handle.write(&freeent, fat_block->getOffset(cluster, i),
                                          fat_block->m_cluster_size);
    }

    optional<ino_t> FATDirectory::getAssoc(const char* filename) {
        for (int i = 0; i < m_assocs.count(); i++)
            if (memcmp(filename, m_assocs[i].filename, 11) == 0)
                return m_assocs[i].id;

        return {};
    }

    optional<FATDirectory::inode_assoc> FATDirectory::getAssoc(ino_t id) {
        for (int i = 0; i < m_assocs.count(); i++)
            if (m_assocs[i].id == id)
                return m_assocs[i];

        return {};
    }

    void FATDirectory::pushAssoc(const char* filename, ino_t id) {
        inode_assoc assoc;

        strlcpy(assoc.filename, filename, 12);
        assoc.id = id;

        m_assocs.push(assoc);
    }

    void FATDirectory::eraseAssoc(const char* filename) {
        for (int i = 0; i < m_assocs.count(); i++)
            if (memcmp(filename, m_assocs[i].filename, 11) == 0)
                m_assocs.erase(i--);
    }

    ino_t FATDirectory::createAssoc(fat_dirent dirent) {
        auto fat_block = (FATControlBlock*) controlblock;
        SCOPE(fat_block->m_mutex);

        uint32_t first_cluster = dirent.first_cluster_lo | (dirent.first_cluster_hi << 16);
        ino_t    inode_id      = fat_block->nextIno();
        INode_SP inode;

        if (dirent.attributes & FAT_DIRECTORY) {
            auto dir = new FATDirectory();

            if (first_cluster) {
                fat_block->fillChain(first_cluster, dir->chain());
                dir->m_filled = true;
            }

            // strlcpy(dir->m_name, dirent.filename, 12);

            dir->size         = dir->chain().count() * fat_block->block_size;
            dir->block_count  = dir->chain().count();
            dir->block_size   = fat_block->block_size;
            dir->controlblock = controlblock;

            inode = INode_SP(dir);
        }
        else {
            auto file = new FATFile();

            if (first_cluster) {
                file->chain().first(first_cluster);
                file->m_filled = false;
            }

            // strlcpy(file->m_name, dirent.filename, 12);

            file->size         = dirent.size;
            file->block_size   = fat_block->block_size;
            file->controlblock = controlblock;

            inode = INode_SP(file);
        }

        fat_block->m_cache.set(inode_id, inode);
        pushAssoc(dirent.filename, inode_id);

        return inode_id;
    }
}