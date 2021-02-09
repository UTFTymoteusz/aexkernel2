#pragma once

#include "aex/spinlock.hpp"

#include <stddef.h>
#include <stdint.h>

extern const int m_page_present, m_page_write, m_page_user, m_page_through, m_page_nocache,
    m_page_combine, m_page_global, m_page_nophys, m_page_exec, m_page_fixed, m_page_arbitrary;

// Specifies if a page is present.
#define PAGE_PRESENT m_page_present
// Specifies if a page is writeable.
#define PAGE_WRITE m_page_write
// Specifies if a page is accessible by userspace code.
#define PAGE_USER m_page_user
// Write-through mode.
#define PAGE_THROUGH m_page_through
// Disables caching for a page.
#define PAGE_NOCACHE m_page_nocache
// Write-combining mode.
#define PAGE_COMBINE m_page_combine
// Makes the page not get flushed on context switches.
#define PAGE_GLOBAL m_page_global
// Marks the page as not being the owner of a physical memory frame.
#define PAGE_NOPHYS m_page_nophys
// Puts the page in the executable space.
#define PAGE_EXEC m_page_exec
// If set, the pages will be mapped at the source address.
#define PAGE_FIXED m_page_fixed
// If set, the pages will raise page faults.
#define PAGE_ARBITRARY m_page_arbitrary

namespace AEX::Mem {
    typedef size_t phys_addr;

    /**
     * The pagemap class. Contains the methods required to allocate virtual memory.
     **/
    class Pagemap {
        public:
        void* vstart;
        void* vend;

        phys_addr pageRoot;

        // Flags that will be applied to every mapping under this pagemap;
        uint32_t gflags;

        Pagemap();
        Pagemap(phys_addr pageRoot);
        Pagemap(size_t start, size_t end);

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
        void* allocContinuous(size_t bytes) {
            return allocContinuous(bytes, 0);
        }

        /**
         * Allocates enough physically contiguous pages to fit the specified size and
         * zeroes them out.
         * @param bytes Requested size in bytes.
         * @param flags Optional flags.
         * @param source Optional source address.
         * @returns Virtual address or nullptr on failure.
         **/
        void* allocContinuous(size_t bytes, uint32_t flags, void* source = nullptr);

        /**
         * Maps the specified size to a physical address.
         * @param bytes  Requested size in bytes.
         * @param paddr  The physical address.
         * @param flags  Flags.
         * @param source If PAGE_FIXED is used, source will be the virtual address.
         * @returns Virtual address or nullptr on failure.
         **/
        void* map(size_t bytes, phys_addr paddr, uint16_t flags, void* source = nullptr);

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
        phys_addr paddrof(void* vaddr);

        size_t rawof(void* vaddr);

        Pagemap* fork();
        void     dump();

        private:
        Spinlock m_lock;

        void      assign(int pptr, void* virt, phys_addr phys, uint16_t flags);
        phys_addr unassign(int pptr, void* virt);

        void recache(void* virt, size_t bytes);

        uint64_t* findTable(int pptr, uint64_t virt_addr) {
            uint64_t skip_by;
            return findTable(pptr, virt_addr, &skip_by);
        }
        uint64_t* findTable(int pptr, uint64_t virt_addr, uint64_t* skip_by);

        uint64_t* findTableEnsure(int pptr, uint64_t virt_addr);

        void* findContiguous(int pptr, size_t amount, bool executable = false);
    };

    /**
     * The pagemap that is used by the kernel exclusively.
     **/
    extern Pagemap* kernel_pagemap;

    void init();

    void cleanup_bootstrap();
}