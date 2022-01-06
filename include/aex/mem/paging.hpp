#pragma once

#include "aex/mem/types.hpp"
#include "aex/spinlock.hpp"
#include "aex/utility.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX::Mem {
    API extern const int m_page_present, m_page_write, m_page_user, m_page_through, m_page_nocache,
        m_page_combine, m_page_global, m_page_nophys, m_page_exec, m_page_fixed, m_page_arbitrary;
}

// Specifies if pages are present.
#define PAGE_PRESENT AEX::Mem::m_page_present
// Specifies if pages are writeable.
#define PAGE_WRITE AEX::Mem::m_page_write
// Specifies if pages are accessible from userspace.
#define PAGE_USER AEX::Mem::m_page_user
// Write-through mode.
#define PAGE_THROUGH AEX::Mem::m_page_through
// Disables caching for a page.
#define PAGE_NOCACHE AEX::Mem::m_page_nocache
// Write-combining mode.
#define PAGE_COMBINE AEX::Mem::m_page_combine
// Makes the pages not get flushed on context switches.
#define PAGE_GLOBAL AEX::Mem::m_page_global
// Marks the pages as not being the owner of a physical memory frame.
#define PAGE_NOPHYS AEX::Mem::m_page_nophys
// Puts the pages in the executable space.
#define PAGE_EXEC AEX::Mem::m_page_exec
// If set, the pages will be mapped at the source address.
#define PAGE_FIXED AEX::Mem::m_page_fixed
// If set, the pages will raise page faults.
#define PAGE_ARBITRARY AEX::Mem::m_page_arbitrary

namespace AEX::Mem {
    /**
     * The pagemap class. Contains the methods required to allocate virtual memory.
     **/
    class API Pagemap {
        public:
        virt_t vstart;
        virt_t vend;

        phys_t root;

        // Flags that will be applied to every mapping under this pagemap;
        uint32_t gflags;

        Pagemap();
        Pagemap(phys_t root);
        Pagemap(virt_t start, virt_t end);
        ~Pagemap();

        /**
         * Allocates enough pages to fit the specified size and zeroes them out.
         * @param bytes Requested size in bytes.
         * @returns Virtual address or nullptr on failure.
         **/
        void* alloc(size_t bytes) {
            return alloc(bytes, 0);
        }

        /**
         * Allocates enough pages to fit the specified size and zeroes them out.
         * @param bytes Requested size in bytes.
         * @param flags Optional flags.
         * @param source Optional source address.
         * @returns Virtual address or nullptr on failure.
         **/
        void* alloc(size_t bytes, uint32_t flags, void* source = nullptr);

        /**
         * Allocates enough physically contiguous pages to fit the specified size and
         * zeroes them out.
         * @param bytes Requested size in bytes.
         * @returns Virtual address or nullptr on failure.
         **/
        void* pcalloc(size_t bytes) {
            return pcalloc(bytes, 0);
        }

        /**
         * Allocates enough physically contiguous pages to fit the specified size and
         * zeroes them out.
         * @param bytes Requested size in bytes.
         * @param flags Optional flags.
         * @param source Optional source address.
         * @returns Virtual address or nullptr on failure.
         **/
        void* pcalloc(size_t bytes, uint32_t flags, void* source = nullptr);

        /**
         * Maps the specified size to a physical address.
         * @param bytes  Requested size in bytes.
         * @param paddr  The physical address.
         * @param flags  Flags.
         * @param source If PAGE_FIXED is used, source will be the virtual address.
         * @returns Virtual address or nullptr on failure.
         **/
        void* map(size_t bytes, phys_t paddr, uint16_t flags, void* source = nullptr);

        /**
         * Frees the specified size from the address space. Automatically frees physical memory.
         * @param virt  Virtual address.
         * @param bytes Size.
         **/
        void free(void* addr, size_t bytes);

        /**
         * Gets the physical address of a virtual address. Respects non-page-aligned addresses.
         * @param vaddr The virtual address.
         * @returns The physical address or 0 on failure.
         **/
        phys_t paddrof(void* vaddr);
        size_t rawof(void* vaddr);

        Pagemap* fork();
        void     dump();

        private:
        Spinlock m_lock;

        void   assign(int pptr, virt_t virt, phys_t phys, uint16_t flags);
        phys_t unassign(int pptr, virt_t virt);

        void recache(virt_t virt, size_t bytes);

        uint64_t* findTable(int pptr, virt_t virt_addr) {
            uint64_t skip_by;
            return findTable(pptr, virt_addr, &skip_by);
        }

        uint64_t* findTable(int pptr, virt_t virt_addr, uint64_t* skip_by);
        uint64_t* findTableEnsure(int pptr, virt_t virt_addr);

        void* findContiguous(int pptr, size_t amount, bool executable = false);

        phys_t paddrof_internal(int pptr, virt_t vaddr);
        size_t rawof_internal(int pptr, virt_t vaddr);
    };

    /**
     * The pagemap that is used by the kernel exclusively.
     **/
    API extern Pagemap* kernel_pagemap;

    void init();
    void cleanup();
}