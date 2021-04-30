#include "aex/mem/paging.hpp"

#include "aex/arch/sys/cpu.hpp"
#include "aex/math.hpp"
#include "aex/mem.hpp"
#include "aex/mutex.hpp"
#include "aex/printk.hpp"
#include "aex/string.hpp"

#include "mem/sections.hpp"
#include "proc/proc.hpp"

#include <stddef.h>
#include <stdint.h>

using namespace AEX::Mem::Phys;

constexpr auto PPTR_AMOUNT   = 256;
constexpr auto MEM_PAGE_MASK = ~0xFFF;

extern void* pml4;

namespace AEX::Mem {
    const int m_page_present   = 0x01;
    const int m_page_write     = 0x02;
    const int m_page_user      = 0x04;
    const int m_page_through   = 0x08;
    const int m_page_nocache   = 0x10;
    const int m_page_pat       = 0x80;
    const int m_page_combine   = m_page_pat;
    const int m_page_global    = 0x100;
    const int m_page_nophys    = 0x200;
    const int m_page_exec      = 0x1000;
    const int m_page_fixed     = 0x2000;
    const int m_page_arbitrary = 0x4000;

    Pagemap* kernel_pagemap;
    Pagemap  m_kernel_pagemap = {0};

    uint64_t* pptr_entries[PPTR_AMOUNT];
    uint64_t* pptr_vaddr[PPTR_AMOUNT];
    bool      pptr_taken[PPTR_AMOUNT];
    size_t    debug_pptr_targets[PPTR_AMOUNT];

    bool ready = false;

    Spinlock pptr_lock;

    int alloc_pptr() {
        int tries = 0;

        while (true) {
            pptr_lock.acquire();

            for (size_t i = 0; i < PPTR_AMOUNT; i++) {
                if (pptr_taken[i])
                    continue;

                pptr_taken[i] = true;
                pptr_lock.release();

                return i;
            }

            pptr_lock.release();

            if (tries++ > 1000)
                kpanic("alloc_pptr is stuck");
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

        Sys::CPU::flushPg(pptr_vaddr[index]);

        return pptr_vaddr[index];
    }

    uint64_t* find_table(phys_addr root, int pptr, uint64_t virt_addr, uint64_t* skip_by) {
        uint64_t pml4index = (virt_addr >> 39) & 0x1FF;
        uint64_t pdpindex  = (virt_addr >> 30) & 0x1FF;
        uint64_t pdindex   = (virt_addr >> 21) & 0x1FF;

        auto pml4 = aim_pptr(pptr, root);
        if (!pml4[pml4index]) {
            *skip_by += 0x8000000000;
            return nullptr;
        }

        auto pdp = aim_pptr(pptr, pml4[pml4index] & ~0xFFF);
        if (!pdp[pdpindex]) {
            *skip_by += 0x40000000;
            return nullptr;
        }

        auto pd = aim_pptr(pptr, pdp[pdpindex] & ~0xFFF);
        if (!pd[pdindex]) {
            *skip_by += 0x200000;
            return nullptr;
        }

        auto ptable = aim_pptr(pptr, pd[pdindex] & ~0xFFF);

        return (uint64_t*) ((uint64_t) ptable & ~0xFFF);
    }

    Pagemap::Pagemap() {
        int pptr_a = alloc_pptr();
        int pptr_b = alloc_pptr();

        m_lock.acquire();

        pageRoot = Phys::alloc(1);

        uint64_t* a = aim_pptr(pptr_a, pageRoot);
        uint64_t* b = aim_pptr(pptr_b, kernel_pagemap->pageRoot);

        memset64(a, 0x0000, 256);
        memcpy(&a[256], &b[256], sizeof(uint64_t) * 256);

        m_lock.release();

        free_pptr(pptr_b);
        free_pptr(pptr_a);
    }

    Pagemap::Pagemap(phys_addr rootAddr) {
        pageRoot = rootAddr;

        vstart = (void*) 0x0000000000000000;
        vend   = (void*) 0x00007FFFFFFFFFFF;

        gflags = 0;
    }

    Pagemap::Pagemap(size_t start, size_t end) {
        int pptr_a = alloc_pptr();
        int pptr_b = alloc_pptr();

        m_lock.acquire();

        pageRoot = Phys::alloc(1);

        uint64_t* a = aim_pptr(pptr_a, pageRoot);
        uint64_t* b = aim_pptr(pptr_b, kernel_pagemap->pageRoot);

        memset64(a, 0x0000, 256);
        memcpy(&a[256], &b[256], sizeof(uint64_t) * 256);

        vstart = (void*) start;
        vend   = (void*) end;

        m_lock.release();

        free_pptr(pptr_b);
        free_pptr(pptr_a);
    }

    Pagemap::~Pagemap() {
        int pptr_a = alloc_pptr();
        int pptr_b = alloc_pptr();
        int pptr_c = alloc_pptr();
        int pptr_d = alloc_pptr();
        m_lock.acquire();

        auto pml4 = aim_pptr(pptr_a, pageRoot);

        for (size_t pdp_i = (size_t) vstart / 0x1000000000000;
             pdp_i < (size_t) vend / 0x1000000000000 + 1; pdp_i++) {
            if (!pml4[pdp_i])
                continue;

            auto pdp = aim_pptr(pptr_b, pml4[pdp_i]);

            for (int pd_i = 0; pd_i < 512; pd_i++) {
                if (!pdp[pd_i])
                    continue;

                auto pd = aim_pptr(pptr_c, pdp[pd_i]);

                for (int pt_i = 0; pt_i < 512; pt_i++) {
                    if (!pd[pt_i])
                        continue;

                    auto pt = aim_pptr(pptr_d, pd[pt_i]);

                    for (int pg_i = 0; pg_i < 512; pg_i++) {
                        if (pt[pg_i] == 0x0000 || pt[pg_i] == 0x00AA000000000000)
                            continue;

                        if (pt[pg_i] & PAGE_NOPHYS)
                            continue;

                        Phys::free(pt[pg_i] & MEM_PAGE_MASK, 1);
                    }
                    Phys::free(pd[pt_i] & MEM_PAGE_MASK, 1);
                }
                Phys::free(pdp[pd_i] & MEM_PAGE_MASK, 1);
            }
            Phys::free(pml4[pdp_i] & MEM_PAGE_MASK, 1);
        }
        Phys::free(pageRoot, 1);

        AEX_ASSERT(Sys::CPU::checkInterrupts());
        Sys::CPU::broadcast(Sys::CPU::IPP_PG_FLUSH);

        m_lock.release();
        free_pptr(pptr_d);
        free_pptr(pptr_c);
        free_pptr(pptr_b);
        free_pptr(pptr_a);
    }

    void* Pagemap::alloc(size_t bytes, uint32_t flags, void* source) {
        if (bytes == 0)
            return nullptr;

        int pptr = alloc_pptr();
        m_lock.acquire();

        size_t amount = ceiltopg(bytes);
        size_t vaddr  = (size_t)(
            (flags & PAGE_FIXED) ? source : findContiguous(pptr, amount, flags & PAGE_EXEC));
        size_t start = vaddr;

        if (vaddr == 0) {
            kpanic("aa");
        }

        flags |= PAGE_PRESENT;
        flags |= gflags;
        flags &= 0xFFF;

        for (size_t i = 0; i < amount; i++) {
            phys_addr phys = AEX::Mem::Phys::alloc(Sys::CPU::PAGE_SIZE);
            memset64(aim_pptr(pptr, phys), 0x0000, 512);

            assign(pptr, (void*) vaddr, phys, flags);

            vaddr += Sys::CPU::PAGE_SIZE;
        }

        // proot->frames_used += amount;
        recache((void*) start, bytes);

        m_lock.release();
        free_pptr(pptr);

        return (void*) start;
    }

    void* Pagemap::allocContinuous(size_t bytes, uint32_t flags, void* source) {
        if (bytes == 0)
            return nullptr;

        int pptr = alloc_pptr();
        m_lock.acquire();

        size_t amount = ceiltopg(bytes);
        size_t vaddr  = (size_t)(
            (flags & PAGE_FIXED) ? source : findContiguous(pptr, amount, flags & PAGE_EXEC));
        size_t paddr = AEX::Mem::Phys::alloc(bytes);
        size_t start = vaddr;

        flags |= PAGE_PRESENT;
        flags |= gflags;
        flags &= 0xFFF;

        for (size_t i = 0; i < amount; i++) {
            memset64(aim_pptr(pptr, paddr), 0x0000, 4096 / sizeof(uint64_t));

            assign(pptr, (void*) vaddr, paddr, flags);

            vaddr += Sys::CPU::PAGE_SIZE;
            paddr += Sys::CPU::PAGE_SIZE;
        }

        recache((void*) start, bytes);

        m_lock.release();
        free_pptr(pptr);

        return (void*) start;
    }

    void* Pagemap::map(size_t bytes, phys_addr paddr, uint16_t flags, void* source) {
        flags |= PAGE_NOPHYS | PAGE_PRESENT;

        int pptr = alloc_pptr();
        m_lock.acquire();

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

        flags |= gflags;
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

        recache((void*) start, bytes + Sys::CPU::PAGE_SIZE * !!offset);

        m_lock.release();
        free_pptr(pptr);

        return (void*) (start + offset);
    }

    void Pagemap::free(void* addr, size_t bytes) {
        int pptr = alloc_pptr();
        m_lock.acquire();

        size_t amount = ceiltopg(bytes);
        size_t vaddr  = (size_t) addr;

        phys_addr free_buff[256];
        int       free_ptr = 0;

        auto free_phys = [this, addr, bytes, &free_buff, &free_ptr]() {
            for (int i = 0; i < free_ptr; i++)
                Mem::Phys::free(free_buff[i], Sys::CPU::PAGE_SIZE);

            free_ptr = 0;
            recache(addr, bytes);
        };

        for (size_t i = 0; i < amount; i++) {
            {
                phys_addr raw = rawof_internal(pptr, (void*) vaddr);

                if (raw && (raw & MEM_PAGE_MASK) != 0x00AA000000000000 && !(raw & PAGE_NOPHYS))
                    memset64(aim_pptr(pptr, raw & MEM_PAGE_MASK), 0x5A, 4096 / sizeof(uint64_t));
            }

            phys_addr addr = unassign(pptr, (void*) vaddr);

            if (addr == 0x0000) {
                vaddr += Sys::CPU::PAGE_SIZE;
                continue;
            }
            free_buff[free_ptr] = addr;
            free_ptr++;

            if (free_ptr == 256)
                free_phys();

            vaddr += Sys::CPU::PAGE_SIZE;
        }

        free_phys();

        m_lock.release();
        free_pptr(pptr);
    }

    size_t Pagemap::rawof(void* addr) {
        uint64_t vaddr = (uint64_t) addr;
        uint64_t index = ((size_t) addr >> 12) & 0x1FF;

        vaddr &= ~0xFFF;

        int pptr = alloc_pptr();
        // m_lock.acquire();

        uint64_t* table = findTable(pptr, vaddr);
        if (!table) {
            m_lock.release();
            free_pptr(pptr);

            return 0;
        }

        uint64_t boi = table[index];

        // m_lock.release();
        free_pptr(pptr);

        return boi;
    }

    Pagemap* Pagemap::fork() {
        int pptrtsrc = alloc_pptr();
        int pptrtdst = alloc_pptr();
        int pptrsrc  = alloc_pptr();
        int pptrdst  = alloc_pptr();

        m_lock.acquire();

        size_t vaddr = 0x0000;

        auto child = new Pagemap((size_t) this->vstart, (size_t) this->vend);

        while (vaddr < (size_t) this->vend) {
            size_t* srctbl = find_table(this->pageRoot, pptrtsrc, vaddr, &vaddr);
            if (!srctbl)
                continue;

            size_t* dsttbl = child->findTableEnsure(pptrtdst, vaddr);

            memset64(dsttbl, 0x0000, 512);

            for (size_t i = 0; i < 512; i++) {
                if (!srctbl[i]) {
                    vaddr += Sys::CPU::PAGE_SIZE;
                    continue;
                }

                size_t srcaddr  = srctbl[i] & MEM_PAGE_MASK;
                size_t srcflags = srctbl[i] & ~MEM_PAGE_MASK;

                if (srcflags & PAGE_NOPHYS) {
                    dsttbl[i] = srctbl[i];
                    vaddr += Sys::CPU::PAGE_SIZE;

                    continue;
                }

                size_t dstaddr = Mem::Phys::alloc(Sys::CPU::PAGE_SIZE);

                memcpy(aim_pptr(pptrdst, dstaddr), aim_pptr(pptrsrc, srcaddr), Sys::CPU::PAGE_SIZE);

                dsttbl[i] = dstaddr | srcflags;

                vaddr += Sys::CPU::PAGE_SIZE;
            }
        }

        m_lock.release();

        free_pptr(pptrdst);
        free_pptr(pptrsrc);
        free_pptr(pptrtdst);
        free_pptr(pptrtsrc);

        return child;
    }

    void Pagemap::dump() {
        int    pptrtsrc = alloc_pptr();
        size_t vaddr    = 0x0000;

        while (vaddr < (size_t) this->vend) {
            size_t* srctbl = find_table(this->pageRoot, pptrtsrc, vaddr, &vaddr);
            if (!srctbl)
                continue;

            for (size_t i = 0; i < 512; i++) {
                if (srctbl[i])
                    printk("0x%p >> 0x%p\n", vaddr, srctbl[i]);

                vaddr += Sys::CPU::PAGE_SIZE;
            }
        }

        free_pptr(pptrtsrc);
    }

    phys_addr Pagemap::paddrof(void* addr) {
        int pptr = alloc_pptr();
        m_lock.acquire();

        uint64_t paddr = paddrof_internal(pptr, addr);
        
        m_lock.release();
        free_pptr(pptr);

        return paddr;
    }

    // Private
    void Pagemap::assign(int pptr, void* virt, phys_addr phys, uint16_t flags) {
        AEX_ASSERT_PEDANTIC(!m_lock.tryAcquire());

        uint64_t vaddr = (uint64_t) virt & MEM_PAGE_MASK;
        uint64_t paddr = (uint64_t) phys & MEM_PAGE_MASK;

        uint64_t* ptable = findTableEnsure(pptr, vaddr);
        uint64_t  index  = (vaddr >> 12) & 0x1FF;

        ptable[index] = paddr | flags;

        __sync_synchronize();
    }

    // Make this unalloc page tables pls
    phys_addr Pagemap::unassign(int pptr, void* virt) {
        AEX_ASSERT(virt >= vstart);
        AEX_ASSERT(virt <= vend);
        AEX_ASSERT_PEDANTIC(!m_lock.tryAcquire());

        uint64_t  vaddr  = (uint64_t) virt & MEM_PAGE_MASK;
        uint64_t* ptable = findTableEnsure(pptr, vaddr);
        uint64_t  index  = (vaddr >> 12) & 0x1FF;

        if (ptable[index] & PAGE_NOPHYS) {
            ptable[index] = 0x0000;
            return 0x0000;
        }

        size_t addr = ptable[index] & MEM_PAGE_MASK;

        ptable[index] = 0x0000;
        __sync_synchronize();

        return addr;
    }

    struct invm_data {
        size_t   addr;
        uint32_t pages;
    };

    void Pagemap::recache(void* addr, size_t bytes) {
        invm_data bong = {
            .addr  = (size_t) addr,
            .pages = ceiltopg<uint32_t>(bytes),
        };

        Sys::CPU::broadcast(Sys::CPU::IPP_PG_INVM, &bong, false);
    }

    uint64_t* Pagemap::findTable(int pptr, uint64_t virt_addr, uint64_t* skip_by) {
        return find_table(pageRoot, pptr, virt_addr, skip_by);
    }

    uint64_t* Pagemap::findTableEnsure(int pptr, uint64_t virt_addr) {
        uint8_t index_shift = 39;

        uint64_t* ptable = aim_pptr(pptr, pageRoot);

        // Here we iterate over 3 paging levels, the PML4, the PDP and the PD
        // until we reach the PT
        for (int i = 0; i < 3; i++) {
            bool     fresh = false;
            uint64_t index = (virt_addr >> index_shift) & 0x01FF;
            index_shift -= 9;

            if (!(ptable[index] & PAGE_PRESENT)) {
                phys_addr phys = AEX::Mem::Phys::alloc(Sys::CPU::PAGE_SIZE);

                ptable[index] = phys;
                fresh         = true;
            }

            ptable[index] |= PAGE_USER | PAGE_WRITE | PAGE_PRESENT;

            auto next_ptable = aim_pptr(pptr, ptable[index] & MEM_PAGE_MASK);
            if (fresh)
                memset64((void*) next_ptable, 0x0000, 512);

            ptable = next_ptable;
        }

        return (uint64_t*) ((uint64_t) ptable & MEM_PAGE_MASK);
    }

    void* Pagemap::findContiguous(int pptr, size_t amount, bool executable) {
        if (amount == 0)
            return nullptr;

        size_t m_vstart = (size_t) vstart;
        size_t m_vend   = (size_t) vend;

        if (executable) {
            if (m_vstart == 0xFFFF800000000000)
                m_vstart = 0xFFFFFFFF80000000;
            else if (m_vend == 0x00007FFFFFFFFFFF)
                m_vstart = 0x00007FFF80000000;
        }

        uint64_t index = ((size_t) m_vstart >> 12) & 0x1FF;
        size_t   vaddr = m_vstart;
        size_t   combo = 0;
        void*    start = (void*) m_vstart;

        uint64_t* tb = findTable(pptr, vaddr);
        if (tb == nullptr)
            tb = findTableEnsure(pptr, vaddr);

        while (vaddr < m_vend) {
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

    phys_addr Pagemap::paddrof_internal(int pptr, void* addr) {
        uint64_t vaddr  = (uint64_t) addr;
        uint16_t offset = vaddr & 0xFFF;
        uint64_t index  = ((size_t) addr >> 12) & 0x1FF;

        vaddr &= ~0xFFF;

        uint64_t* table = findTable(pptr, vaddr);
        if (!table || (table[index] & MEM_PAGE_MASK) == 0)
            return 0x0000;

        uint64_t paddr = (table[index] & MEM_PAGE_MASK) + offset;

        return paddr;
    }

    size_t Pagemap::rawof_internal(int pptr, void* addr) {
        uint64_t vaddr = (uint64_t) addr;
        uint64_t index = ((size_t) addr >> 12) & 0x1FF;

        vaddr &= ~0xFFF;

        uint64_t* table = findTable(pptr, vaddr);
        if (!table)
            return 0;

        return table[index];
    }

    void create_first_levels();

    void init() {
        new (&m_kernel_pagemap) Pagemap((phys_addr) &pml4);

        m_kernel_pagemap.vstart = (void*) 0xFFFF800000000000;
        m_kernel_pagemap.vend   = (void*) 0xFFFFFFFFFFFFFFFF;

        m_kernel_pagemap.gflags = 0;

        kernel_pagemap = &m_kernel_pagemap;

        uint64_t* prev_ptable = nullptr;

        for (size_t i = 0; i < PPTR_AMOUNT; i++) {
            void*    m_virt = m_kernel_pagemap.map(Sys::CPU::PAGE_SIZE, 0x0000, PAGE_WRITE);
            uint64_t virt   = (uint64_t) m_virt;

            uint64_t pml4index = (virt >> 39) & 0x1FF;
            uint64_t pdpindex  = (virt >> 30) & 0x1FF;
            uint64_t pdindex   = (virt >> 21) & 0x1FF;
            uint64_t ptindex   = (virt >> 12) & 0x1FF;

            uint64_t* pml4   = (uint64_t*) m_kernel_pagemap.pageRoot;
            uint64_t* pdp    = (uint64_t*) (pml4[pml4index] & ~0xFFF);
            uint64_t* pd     = (uint64_t*) (pdp[pdpindex] & ~0xFFF);
            uint64_t* ptable = (uint64_t*) (pd[pdindex] & ~0xFFF);

            uint64_t* vpt;

            if (ptable != prev_ptable)
                vpt = (uint64_t*) m_kernel_pagemap.map(Sys::CPU::PAGE_SIZE, (phys_addr) ptable,
                                                       PAGE_WRITE);
            else
                vpt = prev_ptable;

            pptr_entries[i] = (uint64_t*) (vpt + ptindex);
            pptr_vaddr[i]   = (uint64_t*) m_virt;

            prev_ptable = vpt;
        }

        ready = true;

        create_first_levels();
    }

    void cleanup() {
        int       pptr  = alloc_pptr();
        uint64_t* table = find_table(Mem::kernel_pagemap->pageRoot, pptr, 0x0000, nullptr);

        memset(table, '\0', 4096);

        free_pptr(pptr);

        Sys::CPU::broadcast(Sys::CPU::IPP_PG_FLUSH, nullptr);

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