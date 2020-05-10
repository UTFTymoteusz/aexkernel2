#include "aex/mem/vmem.hpp"

#include "aex/kpanic.hpp"
#include "aex/mem/pmem.hpp"
#include "aex/printk.hpp"
#include "aex/spinlock.hpp"
#include "aex/string.hpp"

#include "mem/memory.hpp"
#include "sys/cpu.hpp"

#include <stdint.h>

#define PPTR_AMOUNT 16

#define MEM_PAGE_MASK ~0xFFF

using namespace AEX::PMem;

extern void* pml4;

int _page_present = 0x01;
int _page_write   = 0x02;
int _page_user    = 0x04;
int _page_nocache = 0x10;
int _page_nophys  = 0x200;

namespace AEX::VMem {
    Pagemap* kernel_pagemap;
    Pagemap  _kernel_pagemap = {0};

    uint64_t* pptr_entries[PPTR_AMOUNT];
    uint64_t* pptr_vaddr[PPTR_AMOUNT];
    bool      pptr_taken[PPTR_AMOUNT];
    size_t    debug_pptr_targets[PPTR_AMOUNT];

    bool ready = false;

    Spinlock pptr_lock;

    int alloc_pptr() {
        while (true) {
            pptr_lock.acquire();

            for (size_t i = 0; i < PPTR_AMOUNT; i++)
                if (!pptr_taken[i]) {
                    pptr_taken[i] = true;
                    pptr_lock.release();

                    return i;
                }

            // yield here
            pptr_lock.release();
        }
    }

    void free_pptr(int index) {
        pptr_lock.acquire();
        pptr_taken[index] = false;
        pptr_lock.release();
    }

    uint64_t* aim_pptr(int index, phys_addr at) {
        if (!ready)
            return (uint64_t*) ((size_t) &KERNEL_VMA + at);

        debug_pptr_targets[index] = at;
        *pptr_entries[index]      = at | PAGE_WRITE | PAGE_PRESENT;

        size_t aa = 0;
        asm volatile("mov %0, cr3; mov cr3, %0;" : : "r"(aa) : "memory");

        return pptr_vaddr[index];
    }

    Pagemap::Pagemap(phys_addr rootAddr) {
        pageRoot = rootAddr;
    }

    void* Pagemap::alloc(size_t bytes, uint32_t flags) {
        if (bytes == 0)
            return nullptr;

        size_t amount = ceiltopg(bytes);

        flags &= 0xFFF;

        spinlock.acquire();

        int    pptr  = alloc_pptr();
        size_t virt  = (size_t) findContiguous(pptr, amount);
        size_t start = virt;

        phys_addr phys;

        for (size_t i = 0; i < amount; i++) {
            phys = AEX::PMem::alloc(Sys::CPU::PAGE_SIZE);

            // printk("alloc 0x%016x >> 0x%016x\n", virt, phys);

            assign(pptr, (void*) virt, phys, flags);

            virt += Sys::CPU::PAGE_SIZE;
        }

        // proot->frames_used += amount;
        free_pptr(pptr);
        spinlock.release();

        memset((void*) start, 0, amount * Sys::CPU::PAGE_SIZE);

        return (void*) start;
    }

    void* Pagemap::allocContinuous(size_t bytes, uint32_t flags) {
        if (bytes == 0)
            return nullptr;

        size_t amount = ceiltopg(bytes);

        flags &= 0xFFF;

        spinlock.acquire();

        int    pptr  = alloc_pptr();
        size_t virt  = (size_t) findContiguous(pptr, amount);
        size_t phys  = AEX::PMem::alloc(bytes);
        size_t start = virt;

        for (size_t i = 0; i < amount; i++) {
            // printk("alloc 0x%016x >> 0x%016x\n", virt, phys);

            assign(pptr, (void*) virt, phys, flags);

            virt += Sys::CPU::PAGE_SIZE;
            phys += Sys::CPU::PAGE_SIZE;
        }

        // proot->frames_used += amount;
        free_pptr(pptr);
        spinlock.release();

        memset((void*) start, 0, amount * Sys::CPU::PAGE_SIZE);

        return (void*) start;
    }

    void* Pagemap::map(size_t bytes, phys_addr paddr, uint16_t flags) {
        flags &= 0x0FFF;

        size_t    amount = ceiltopg(bytes);
        phys_addr offset = 0;

        if ((paddr & 0xFFF) > 0) {
            amount++;
            offset = paddr & 0xFFF;
        }

        spinlock.acquire();

        int    pptr  = alloc_pptr();
        size_t virt  = (size_t) findContiguous(pptr, amount);
        size_t start = virt;

        for (size_t i = 0; i < amount; i++) {
            assign(pptr, (void*) virt, paddr - offset, flags | PAGE_NOPHYS);

            // printk("map 0x%016p >> 0x%016x (+0x%04x)\n", virt, paddr - offset, offset);

            paddr += Sys::CPU::PAGE_SIZE;
            virt += Sys::CPU::PAGE_SIZE;
        }

        free_pptr(pptr);
        spinlock.release();

        return (void*) (start + offset);
    }

    // Private
    void Pagemap::assign(int pptr, void* virt, phys_addr phys, uint16_t flags) {
        uint64_t virt_addr = (uint64_t) virt;
        virt_addr &= MEM_PAGE_MASK;

        uint64_t phys_addr = (uint64_t) phys;
        phys_addr &= MEM_PAGE_MASK;

        uint64_t* pt    = findTableEnsure(pptr, virt_addr);
        uint64_t  index = (virt_addr >> 12) & 0x1FF;

        pt[index] = phys_addr | flags | PAGE_PRESENT;

        size_t aa = 0;
        asm volatile("mov %0, cr3; mov cr3, %0;" : : "r"(aa) : "memory");
    }

    uint64_t* find_table(phys_addr root, int pptr, uint64_t virt_addr, uint64_t* skip_by) {
        uint64_t pml4index = (virt_addr >> 39) & 0x1FF;
        uint64_t pdpindex  = (virt_addr >> 30) & 0x1FF;
        uint64_t pdindex   = (virt_addr >> 21) & 0x1FF;

        auto pml4 = aim_pptr(pptr, root);
        if (pml4[pml4index] == 0x0000) {
            *skip_by += 0x8000000000;
            return nullptr;
        }
        auto pdp = aim_pptr(pptr, pml4[pml4index] & ~0xFFF);
        if (pdp[pdpindex] == 0x0000) {
            *skip_by += 0x40000000;
            return nullptr;
        }
        auto pd = aim_pptr(pptr, pdp[pdpindex] & ~0xFFF);
        if (pd[pdindex] == 0x0000) {
            *skip_by += 0x200000;
            return nullptr;
        }
        auto pt = aim_pptr(pptr, pd[pdindex] & ~0xFFF);

        return (uint64_t*) ((uint64_t) pt & ~0xFFF);
    }

    uint64_t* Pagemap::findTable(int pptr, uint64_t virt_addr, uint64_t* skip_by) {
        return find_table(pageRoot, pptr, virt_addr, skip_by);
    }

    uint64_t* Pagemap::findTableEnsure(int pptr, uint64_t virt_addr) {
        uint8_t index_shift = 39;
        bool    reset       = false;

        uint64_t* pt = aim_pptr(pptr, pageRoot);

        // Here we iterate over 3 paging levels, the PML4, the PDP and the PD
        // until we reach the PT
        for (int i = 0; i < 3; i++) {
            uint64_t index = (virt_addr >> index_shift) & 0x01FF;
            index_shift -= 9;

            if (!(pt[index] & PAGE_PRESENT)) {
                phys_addr phys = AEX::PMem::alloc(Sys::CPU::PAGE_SIZE);

                // proot->dir_frames_used++;

                pt[index] = phys | PAGE_USER | PAGE_WRITE | PAGE_PRESENT;
                reset     = true;
            }
            else
                pt[index] |= PAGE_USER | PAGE_WRITE | PAGE_PRESENT;

            pt = aim_pptr(pptr, pt[index] & MEM_PAGE_MASK);

            if (reset) {
                memset((void*) pt, 0, 0x1000);
                reset = false;
            }
        }

        return (uint64_t*) ((uint64_t) pt & MEM_PAGE_MASK);
    }

    void* Pagemap::findContiguous(int pptr, size_t amount) {
        if (amount == 0)
            return nullptr;

        uint64_t index = ((size_t) vstart >> 12) & 0x1FF;
        uint64_t virt  = (uint64_t) vstart;
        void*    start = vstart;
        size_t   combo = 0;

        uint64_t* tb;

        tb = findTable(pptr, virt);
        if (tb == nullptr)
            tb = findTableEnsure(pptr, virt);

        while (true) {
            if (index >= 512) {
                index = 0;

                tb = findTable(pptr, virt);
                if (tb == nullptr) {
                    virt += Sys::CPU::PAGE_SIZE * 512;

                    index = 512;
                    combo += 512;

                    if (combo >= amount)
                        return start;

                    continue;
                }
            }

            if (tb[index++] != 0x0000) {
                virt += Sys::CPU::PAGE_SIZE;

                start = (void*) virt;
                combo = 0;

                continue;
            }

            if (++combo >= amount)
                return start;

            virt += Sys::CPU::PAGE_SIZE;
        }

        return nullptr;
    }

    phys_addr Pagemap::paddrof(void* vaddr) {
        uint64_t virt_addr = (uint64_t) vaddr;
        uint16_t offset    = virt_addr & 0xFFF;
        uint64_t index     = ((size_t) vaddr >> 12) & 0x1FF;

        virt_addr &= ~0xFFF;

        spinlock.acquire();

        int       pptr  = alloc_pptr();
        uint64_t* table = findTable(pptr, virt_addr);
        if (!table)
            return 0;

        uint64_t paddr = (table[index] & MEM_PAGE_MASK) + offset;

        free_pptr(pptr);
        spinlock.release();

        return paddr;
    }

    void init() {
        _kernel_pagemap = Pagemap((phys_addr) &pml4);

        _kernel_pagemap.vstart = (void*) 0xFFFFFFFF80100000;
        _kernel_pagemap.vend   = (void*) 0xFFFFFFFFFFFFFFFF;

        kernel_pagemap = &_kernel_pagemap;

        for (size_t i = 0; i < PPTR_AMOUNT; i++) {
            void* _virt = _kernel_pagemap.map(Sys::CPU::PAGE_SIZE, 0x0000, PAGE_WRITE);

            uint64_t virt = (uint64_t) _virt;

            uint64_t pml4index = (virt >> 39) & 0x1FF;
            uint64_t pdpindex  = (virt >> 30) & 0x1FF;
            uint64_t pdindex   = (virt >> 21) & 0x1FF;
            uint64_t ptindex   = (virt >> 12) & 0x1FF;

            uint64_t* pml4 = (uint64_t*) _kernel_pagemap.pageRoot;
            uint64_t* pdp  = (uint64_t*) (pml4[pml4index] & ~0xFFF);
            uint64_t* pd   = (uint64_t*) (pdp[pdpindex] & ~0xFFF);
            uint64_t* pt   = (uint64_t*) (pd[pdindex] & ~0xFFF);

            uint64_t* vpt =
                (uint64_t*) _kernel_pagemap.map(Sys::CPU::PAGE_SIZE, (phys_addr) pt, PAGE_WRITE);

            pptr_entries[i] = (uint64_t*) (vpt + ptindex);
            pptr_vaddr[i]   = (uint64_t*) _virt;
        }

        ready = true;
    }

    void cleanup_bootstrap() {
        int       pptr  = alloc_pptr();
        uint64_t* table = find_table(VMem::kernel_pagemap->pageRoot, pptr, 0x0000, nullptr);

        memset(table, '\0', 4096);

        free_pptr(pptr);

        Sys::CPU::broadcastPacket(Sys::CPU::ipp_type::PG_FLUSH, nullptr, true);
    }
}