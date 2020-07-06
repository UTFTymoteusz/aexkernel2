#include "aex/mem/paging.hpp"

#include "aex/arch/sys/cpu.hpp"
#include "aex/kpanic.hpp"
#include "aex/math.hpp"
#include "aex/mem.hpp"
#include "aex/mutex.hpp"
#include "aex/printk.hpp"
#include "aex/string.hpp"

#include "mem/sections.hpp"

#include <stddef.h>
#include <stdint.h>

using namespace AEX::Mem::Phys;

constexpr auto PPTR_AMOUNT   = 16;
constexpr auto MEM_PAGE_MASK = ~0xFFF;

extern void* pml4;

const int _page_present   = 0x01;
const int _page_write     = 0x02;
const int _page_user      = 0x04;
const int _page_through   = 0x08;
const int _page_nocache   = 0x10;
const int _page_pat       = 0x80;
const int _page_combine   = _page_pat;
const int _page_nophys    = 0x200;
const int _page_exec      = 0x1000;
const int _page_fixed     = 0x2000;
const int _page_arbitrary = 0x4000;

// I need to make invlpg clump together

namespace AEX::Mem {
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
            return (uint64_t*) ((size_t) &KERNEL_EXEC_VMA + at);

        debug_pptr_targets[index] = at;
        *pptr_entries[index]      = at | PAGE_WRITE | PAGE_PRESENT;

        asm volatile("invlpg [%0]" : : "r"(pptr_vaddr[index]));

        return pptr_vaddr[index];
    }

    Pagemap::Pagemap() {
        pageRoot = Mem::Phys::alloc(1);

        int pptr_a = alloc_pptr();
        int pptr_b = alloc_pptr();

        uint64_t* a = aim_pptr(pptr_a, pageRoot);
        uint64_t* b = aim_pptr(pptr_b, kernel_pagemap->pageRoot);

        memcpy(&a[256], &b[256], sizeof(uint64_t) * 256);

        free_pptr(pptr_b);
        free_pptr(pptr_a);
    }

    Pagemap::Pagemap(phys_addr rootAddr) {
        pageRoot = rootAddr;
    }

    void* Pagemap::alloc(size_t bytes, uint32_t flags) {
        if (bytes == 0)
            return nullptr;

        _lock.acquire();

        int pptr = alloc_pptr();

        size_t amount = ceiltopg(bytes);
        size_t vaddr  = (size_t) findContiguous(pptr, amount, flags & PAGE_EXEC);
        size_t start  = vaddr;

        flags &= 0xFFF;
        flags |= PAGE_PRESENT;

        for (size_t i = 0; i < amount; i++) {
            phys_addr phys = AEX::Mem::Phys::alloc(Sys::CPU::PAGE_SIZE);

            assign(pptr, (void*) vaddr, phys, flags);

            vaddr += Sys::CPU::PAGE_SIZE;
        }

        // proot->frames_used += amount;
        free_pptr(pptr);
        _lock.release();

        memset((void*) start, 0, amount * Sys::CPU::PAGE_SIZE);

        return (void*) start;
    }

    void* Pagemap::allocContinuous(size_t bytes, uint32_t flags) {
        if (bytes == 0)
            return nullptr;

        _lock.acquire();

        int pptr = alloc_pptr();

        size_t amount = ceiltopg(bytes);
        size_t vaddr  = (size_t) findContiguous(pptr, amount, flags & PAGE_EXEC);
        size_t paddr  = AEX::Mem::Phys::alloc(bytes);
        size_t start  = vaddr;

        flags &= 0xFFF;
        flags |= PAGE_PRESENT;

        for (size_t i = 0; i < amount; i++) {
            assign(pptr, (void*) vaddr, paddr, flags);

            vaddr += Sys::CPU::PAGE_SIZE;
            paddr += Sys::CPU::PAGE_SIZE;
        }

        free_pptr(pptr);
        _lock.release();

        memset((void*) start, 0, amount * Sys::CPU::PAGE_SIZE);

        return (void*) start;
    }

    void* Pagemap::map(size_t bytes, phys_addr paddr, uint16_t flags, void* source) {
        flags |= PAGE_NOPHYS | PAGE_PRESENT;

        _lock.acquire();

        int pptr = alloc_pptr();

        size_t    amount = ceiltopg(bytes);
        phys_addr offset = 0;
        size_t    vaddr  = (size_t)((flags & PAGE_FIXED) ? source : findContiguous(pptr, amount));
        size_t    start  = vaddr;

        if ((paddr & 0xFFF) > 0) {
            amount++;
            offset = paddr & 0xFFF;
        }

        bool arbitrary = flags & PAGE_ARBITRARY;
        if (arbitrary)
            flags &= ~PAGE_PRESENT;

        flags &= 0x0FFF;

        if (arbitrary)
            for (size_t i = 0; i < amount; i++) {
                assign(pptr, (void*) vaddr, 0x00AA000000000000, flags);

                vaddr += Sys::CPU::PAGE_SIZE;
            }
        else
            for (size_t i = 0; i < amount; i++) {
                assign(pptr, (void*) vaddr, paddr - offset, flags);

                paddr += Sys::CPU::PAGE_SIZE;
                vaddr += Sys::CPU::PAGE_SIZE;
            }

        free_pptr(pptr);
        _lock.release();

        return (void*) (start + offset);
    }

    void Pagemap::free(void* addr, size_t bytes) {
        _lock.acquire();

        int pptr = alloc_pptr();

        size_t amount = ceiltopg(bytes);
        size_t vaddr  = (size_t) addr;

        for (size_t i = 0; i < amount; i++) {
            unassign(pptr, (void*) vaddr);

            vaddr += Sys::CPU::PAGE_SIZE;
        }

        free_pptr(pptr);
        _lock.release();
    }

    // Private
    void Pagemap::assign(int pptr, void* virt, phys_addr phys, uint16_t flags) {
        uint64_t vaddr = (uint64_t) virt & MEM_PAGE_MASK;
        uint64_t paddr = (uint64_t) phys & MEM_PAGE_MASK;

        uint64_t* ptable = findTableEnsure(pptr, vaddr);
        uint64_t  index  = (vaddr >> 12) & 0x1FF;

        ptable[index] = paddr | flags;

        Sys::CPU::broadcastPacket(Sys::CPU::IPP_PG_INV, virt);

        asm volatile("invlpg [%0]" : : "r"(virt));
    }

    // Make this unalloc page tables pls
    void Pagemap::unassign(int pptr, void* virt) {
        uint64_t  vaddr  = (uint64_t) virt & MEM_PAGE_MASK;
        uint64_t* ptable = findTableEnsure(pptr, vaddr);
        uint64_t  index  = (vaddr >> 12) & 0x1FF;

        if (ptable[index] & PAGE_NOPHYS) {
            ptable[index] = 0x0000;

            Sys::CPU::broadcastPacket(Sys::CPU::IPP_PG_INV, virt);

            asm volatile("invlpg [%0]" : : "r"(virt));
            return;
        }

        size_t addr   = ptable[index] & MEM_PAGE_MASK;
        ptable[index] = 0x0000;

        Sys::CPU::broadcastPacket(Sys::CPU::IPP_PG_INV, virt);
        Mem::Phys::free(addr, Sys::CPU::PAGE_SIZE);

        asm volatile("invlpg [%0]" : : "r"(virt));
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

        auto ptable = aim_pptr(pptr, pd[pdindex] & ~0xFFF);

        return (uint64_t*) ((uint64_t) ptable & ~0xFFF);
    }

    uint64_t* Pagemap::findTable(int pptr, uint64_t virt_addr, uint64_t* skip_by) {
        return find_table(pageRoot, pptr, virt_addr, skip_by);
    }

    uint64_t* Pagemap::findTableEnsure(int pptr, uint64_t virt_addr) {
        uint8_t index_shift = 39;
        bool    reset       = false;

        uint64_t* ptable = aim_pptr(pptr, pageRoot);

        // Here we iterate over 3 paging levels, the PML4, the PDP and the PD
        // until we reach the PT
        for (int i = 0; i < 3; i++) {
            uint64_t index = (virt_addr >> index_shift) & 0x01FF;
            index_shift -= 9;

            if (!(ptable[index] & PAGE_PRESENT)) {
                phys_addr phys = AEX::Mem::Phys::alloc(Sys::CPU::PAGE_SIZE);

                ptable[index] = phys | PAGE_USER | PAGE_WRITE | PAGE_PRESENT;
                reset         = true;
            }
            else
                ptable[index] |= PAGE_USER | PAGE_WRITE | PAGE_PRESENT;

            ptable = aim_pptr(pptr, ptable[index] & MEM_PAGE_MASK);

            if (reset) {
                memset((void*) ptable, 0, 0x1000);
                reset = false;
            }
        }

        return (uint64_t*) ((uint64_t) ptable & MEM_PAGE_MASK);
    }

    void* Pagemap::findContiguous(int pptr, size_t amount, bool executable) {
        if (amount == 0)
            return nullptr;

        size_t _vstart = (size_t) vstart;
        size_t _vend   = (size_t) vend;

        if (executable) {
            if (_vstart == 0xFFFF800000000000)
                _vstart = 0xFFFFFFFF80000000;
            else if (_vend == 0x00007FFFFFFFFFFF)
                _vstart = 0x00007FFF80000000;
        }

        uint64_t index = ((size_t) _vstart >> 12) & 0x1FF;
        size_t   vaddr = _vstart;
        size_t   combo = 0;
        void*    start = (void*) _vstart;

        uint64_t* tb = findTable(pptr, vaddr);
        if (tb == nullptr)
            tb = findTableEnsure(pptr, vaddr);

        while (true) {
            if (index >= 512) {
                index = 0;

                tb = findTable(pptr, vaddr);
                if (tb == nullptr) {
                    vaddr += Sys::CPU::PAGE_SIZE * 512;

                    index = 512;
                    combo += 512;

                    if (combo >= amount)
                        return start;

                    continue;
                }
            }

            if (tb[index++] != 0x0000) {
                vaddr += Sys::CPU::PAGE_SIZE;

                start = (void*) vaddr;
                combo = 0;

                continue;
            }

            if (++combo >= amount)
                return start;

            vaddr += Sys::CPU::PAGE_SIZE;
        }

        return nullptr;
    }

    phys_addr Pagemap::paddrof(void* addr) {
        uint64_t vaddr  = (uint64_t) addr;
        uint16_t offset = vaddr & 0xFFF;
        uint64_t index  = ((size_t) addr >> 12) & 0x1FF;

        vaddr &= ~0xFFF;

        _lock.acquire();

        int       pptr  = alloc_pptr();
        uint64_t* table = findTable(pptr, vaddr);
        if (!table) {
            _lock.release();
            return 0;
        }

        uint64_t paddr = (table[index] & MEM_PAGE_MASK) + offset;

        free_pptr(pptr);
        _lock.release();

        return paddr;
    }

    void create_first_levels();

    void init() {
        _kernel_pagemap = Pagemap((phys_addr) &pml4);

        _kernel_pagemap.vstart = (void*) 0xFFFF800000000000;
        _kernel_pagemap.vend   = (void*) 0xFFFFFFFFFFFFFFFF;

        kernel_pagemap = &_kernel_pagemap;

        for (size_t i = 0; i < PPTR_AMOUNT; i++) {
            void* _virt = _kernel_pagemap.map(Sys::CPU::PAGE_SIZE, 0x0000, PAGE_WRITE);

            uint64_t virt = (uint64_t) _virt;

            uint64_t pml4index = (virt >> 39) & 0x1FF;
            uint64_t pdpindex  = (virt >> 30) & 0x1FF;
            uint64_t pdindex   = (virt >> 21) & 0x1FF;
            uint64_t ptindex   = (virt >> 12) & 0x1FF;

            uint64_t* pml4   = (uint64_t*) _kernel_pagemap.pageRoot;
            uint64_t* pdp    = (uint64_t*) (pml4[pml4index] & ~0xFFF);
            uint64_t* pd     = (uint64_t*) (pdp[pdpindex] & ~0xFFF);
            uint64_t* ptable = (uint64_t*) (pd[pdindex] & ~0xFFF);

            uint64_t* vpt = (uint64_t*) _kernel_pagemap.map(Sys::CPU::PAGE_SIZE, (phys_addr) ptable,
                                                            PAGE_WRITE);

            pptr_entries[i] = (uint64_t*) (vpt + ptindex);
            pptr_vaddr[i]   = (uint64_t*) _virt;
        }

        ready = true;

        create_first_levels();
    }

    void cleanup_bootstrap() {
        int       pptr  = alloc_pptr();
        uint64_t* table = find_table(Mem::kernel_pagemap->pageRoot, pptr, 0x0000, nullptr);

        memset(table, '\0', 4096);

        free_pptr(pptr);

        Sys::CPU::broadcastPacket(Sys::CPU::IPP_PG_FLUSH, nullptr);

        printk(PRINTK_OK "vmem: Bootstrap necessities cleaned\n");
    }

    void create_first_levels() {
        int pptr   = alloc_pptr();
        int pptr_s = alloc_pptr();

        uint64_t* bong = aim_pptr(pptr, kernel_pagemap->pageRoot);

        for (int i = 256; i < 512; i++) {
            if (bong[i])
                continue;

            bong[i] = Mem::Phys::alloc(4096);

            uint64_t* bong_s = aim_pptr(pptr_s, bong[i]);
            memset64(bong_s, 0, 512);
        }

        free_pptr(pptr_s);
        free_pptr(pptr);
    }
}