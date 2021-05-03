#include "aex/mem/mmap.hpp"

#include "aex/assert.hpp"
#include "aex/kpanic.hpp"
#include "aex/proc/process.hpp"
#include "aex/types.hpp"
#include "aex/utility.hpp"

namespace AEX::Mem {
    void remove_region(Proc::Process* process, void* addr);

    Region::Region(Pagemap* pagemap, void* addr, size_t len) {
        this->start = addr;
        this->len   = len;

        this->m_file    = {};
        this->m_pagemap = pagemap;
    }

    Region::Region(Pagemap* pagemap, void* addr, size_t len, FS::File_SP file, int64_t offset) {
        this->start = addr;
        this->len   = len;

        this->m_file    = file;
        this->m_offset  = offset;
        this->m_pagemap = pagemap;
    }

    Region::~Region() {}

    error_t Region::read(void* addr, FS::off_t offset, size_t count) {
        kpanic("Default Region::read(0x%p, %li, %i) called", addr, offset, count);
    }

    optional<Region*> Region::fork(Pagemap* dst) {
        return new Region(dst, start, len);
    }

    FileBackedRegion::FileBackedRegion(Pagemap* pagemap, void* addr, size_t len, FS::File_SP file,
                                       int64_t offset)
        : Region(pagemap, addr, len) {
        this->m_file   = file;
        this->m_offset = offset;

        for (int i = 0; i < CACHE_SLOTS; i++)
            cache[i].buffer = (uint8_t*) kernel_pagemap->alloc(Sys::CPU::PAGE_SIZE, PAGE_WRITE);
    }

    FileBackedRegion::~FileBackedRegion() {
        m_lock.acquire();

        if (m_file)
            m_file.value->close();

        m_pagemap->free(start, len);

        for (int i = 0; i < CACHE_SLOTS; i++)
            kernel_pagemap->free(cache[i].buffer, Sys::CPU::PAGE_SIZE);

        m_lock.release();
    }

    error_t FileBackedRegion::read(void* dst, FS::off_t offset, size_t count) {
        SCOPE(m_lock);

        size_t id = offset / Sys::CPU::PAGE_SIZE;

        int slot = findSlot(id);
        if (slot == -1) {
            printk("failed\n");
            return ENONE;
        }

        int aflags = (m_pagemap != kernel_pagemap) ? PAGE_USER : 0;

        if (cache[slot].id != -1)
            m_pagemap->map(Sys::CPU::PAGE_SIZE, 0, PAGE_FIXED | PAGE_ARBITRARY | aflags,
                           cache[slot].referenced);

        cache[slot].id         = id;
        cache[slot].referenced = dst;

        m_file.value->seek(m_offset + offset);
        m_file.value->read(cache[slot].buffer, count);

        m_pagemap->map(Sys::CPU::PAGE_SIZE, m_pagemap->paddrof(cache[slot].buffer),
                       PAGE_FIXED | aflags, dst);

        // printk("slot: %i for %i\n", slot, id);

        return ENONE;
    }

    optional<Region*> FileBackedRegion::fork(Pagemap* dst) {
        kpanic("FileBackedRegion::fork(0x%p) called", dst);
    }

    int FileBackedRegion::findSlot(int32_t id) {
        int32_t prev_id = id - 1;
        if (id == 0)
            prev_id = 0;

        for (int j = 0; j < CACHE_SLOTS; j++) {
            int i = cache_ptr;

            cache_ptr++;
            if (cache_ptr >= CACHE_SLOTS)
                cache_ptr = 0;

            if (cache[cache_ptr].id == id)
                break;

            if (cache[cache_ptr].id == prev_id)
                continue;

            return i;
        }

        return -1;
    }

    optional<void*> mmap(Proc::Process* process, void* addr, size_t len, int prot, int flags,
                         FS::File_SP file, FS::off_t offset) {
        // make addr be actually used
        if (flags & MAP_FIXED)
            NOT_IMPLEMENTED;

        if (!(flags & MAP_ANONYMOUS) && !file)
            return EBADF;

        AEX_ASSERT(process != nullptr);

        int aflags = (process != Proc::Process::kernel()) ? PAGE_USER : 0;

        if (prot & PROT_WRITE)
            aflags |= PAGE_WRITE;

        Region* region;

        if (flags & MAP_ANONYMOUS) {
            region = new Region(process->pagemap, process->pagemap->alloc(len, aflags), len);

            process->lock.acquire();
            process->mmap_regions.push(region);
            process->lock.release();

            return region->start;
        }

        auto dupd = ENSURE_OPT(file->dup(););
        auto mmap = ENSURE_OPT(dupd->mmap(process, addr, len, aflags, dupd, offset));

        process->lock.acquire();
        process->mmap_regions.push(mmap);
        process->lock.release();

        return mmap->start;
    }

    // TODO: Make len work properly
    error_t munmap(Proc::Process* process, void* addr, size_t) {
        remove_region(process, addr);

        return ENONE;
    }

    Region* find_mmap_region(Proc::Process* process, void* addr) {
        size_t addr_n = (size_t) addr;

        process->lock.acquire();

        for (int i = 0; i < process->mmap_regions.count(); i++) {
            if (!process->mmap_regions.present(i))
                continue;

            auto region = process->mmap_regions[i];

            size_t start = (size_t) region->start;
            size_t len   = region->len;

            if (addr_n < start || addr_n >= start + len)
                continue;

            process->lock.release();
            return region;
        }

        process->lock.release();
        return nullptr;
    }

    void remove_region(Proc::Process* process, void* addr) {
        Region* region = nullptr;
        size_t  addr_n = (size_t) addr;

        process->lock.acquire();

        for (int i = 0; i < process->mmap_regions.count(); i++) {
            if (!process->mmap_regions.present(i))
                continue;

            auto m_region = process->mmap_regions[i];

            size_t start = (size_t) m_region->start;
            size_t len   = m_region->len;

            if (addr_n < start || addr_n >= start + len)
                continue;

            region = m_region;
            process->mmap_regions.erase(i);

            break;
        }

        process->lock.release();

        delete region;
    }
}
