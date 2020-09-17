#include "aex/mem/mmap.hpp"

#include "aex/kpanic.hpp"
#include "aex/proc/process.hpp"

namespace AEX::Mem {
    void remove_region(Proc::Process* process, void* addr);

    MMapRegion::MMapRegion(Pagemap* pagemap, void* addr, size_t len) {
        this->start = addr;
        this->len   = len;

        this->m_file    = {};
        this->m_pagemap = pagemap;
    }

    MMapRegion::~MMapRegion() {}

    error_t MMapRegion::read(void* addr, int64_t offset, uint32_t count) {
        kpanic("Default MMapRegion::read(0x%p, %li, %i) called", addr, offset, count);
    }

    FileBackedMMapRegion::FileBackedMMapRegion(Pagemap* pagemap, void* addr, size_t len,
                                               FS::File_SP file, int64_t offset)
        : MMapRegion(pagemap, addr, len) {
        this->m_file   = file;
        this->m_offset = offset;

        for (int i = 0; i < CACHE_SLOTS; i++)
            cache[i].buffer = (uint8_t*) kernel_pagemap->alloc(Sys::CPU::PAGE_SIZE, PAGE_WRITE);
    }

    FileBackedMMapRegion::~FileBackedMMapRegion() {
        m_lock.acquire();

        if (m_file)
            m_file.value->close();

        m_pagemap->free(start, len);

        for (int i = 0; i < CACHE_SLOTS; i++)
            kernel_pagemap->free(cache[i].buffer, Sys::CPU::PAGE_SIZE);

        m_lock.release();
    }

    error_t FileBackedMMapRegion::read(void* dst, int64_t offset, uint32_t count) {
        ScopeMutex scopeLock(m_lock);

        int32_t id = offset / Sys::CPU::PAGE_SIZE;

        // printk("reading from %li to 0x%p\n", offset, dst);

        int slot = findSlot(id);
        if (slot == -1) {
            printk("failed\n");
            return ENONE;
        }

        if (cache[slot].id != -1)
            m_pagemap->map(Sys::CPU::PAGE_SIZE, 0, PAGE_FIXED | PAGE_ARBITRARY,
                           cache[slot].referenced);

        cache[slot].id         = id;
        cache[slot].referenced = dst;

        m_file.value->seek(m_offset + offset);
        m_file.value->read(cache[slot].buffer, count);

        m_pagemap->map(Sys::CPU::PAGE_SIZE, m_pagemap->paddrof(cache[slot].buffer), PAGE_FIXED,
                       dst);

        // printk("slot: %i for %i\n", slot, id);

        return ENONE;
    }

    int FileBackedMMapRegion::findSlot(int32_t id) {
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

    optional<void*> mmap(void*, size_t len, prot_flags_t, mmap_flags_t flags, FS::File_SP file,
                         int64_t offset) {
        // make addr be actually used

        if (!(flags & MAP_ANONYMOUS) && !file)
            return EBADF;

        auto process = Proc::Process::current();

        MMapRegion* region;

        if (flags & MAP_ANONYMOUS) {
            void* alloc_addr = process->pagemap->alloc(len);

            region = new MMapRegion(process->pagemap, alloc_addr, len);

            process->lock.acquire();
            process->mmap_regions.pushBack(region);
            process->lock.release();

            return region->start;
        }

        auto dupd_try = file->dup();
        if (!dupd_try)
            return dupd_try.error_code;

        auto  dupd       = dupd_try.value;
        void* alloc_addr = process->pagemap->map(len, 0, PAGE_ARBITRARY);

        region = new FileBackedMMapRegion(process->pagemap, alloc_addr, len, dupd, offset);

        process->lock.acquire();
        process->mmap_regions.pushBack(region);
        process->lock.release();

        return region->start;
    }

    // Make len work properly
    error_t munmap(void* addr, size_t) {
        auto process = Proc::Process::current();

        remove_region(process, addr);

        return ENONE;
    }

    Mem::MMapRegion* find_mmap_region(Proc::Process* process, void* addr) {
        size_t addr_n = (size_t) addr;

        process->lock.acquire();

        for (int i = 0; i < process->mmap_regions.count(); i++) {
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
        MMapRegion* region = nullptr;
        size_t      addr_n = (size_t) addr;

        process->lock.acquire();

        for (int i = 0; i < process->mmap_regions.count(); i++) {
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
