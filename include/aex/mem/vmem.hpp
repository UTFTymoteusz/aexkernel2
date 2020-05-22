#pragma once

#include "aex/spinlock.hpp"

#include <stddef.h>
#include <stdint.h>

extern const int _page_present, _page_write, _page_user, _page_through, _page_nocache,
    _page_combine, _page_nophys;

#define PAGE_PRESENT _page_present
#define PAGE_WRITE _page_write
#define PAGE_USER _page_user
#define PAGE_THROUGH _page_through
#define PAGE_NOCACHE _page_nocache
#define PAGE_COMBINE _page_combine
#define PAGE_NOPHYS _page_nophys

namespace AEX::VMem {
    typedef size_t phys_addr;

    /**
     * The pagemap class. Contains the methods required to allocate virtual memory.
     */
    class Pagemap {
      public:
        void* vstart;
        void* vend;

        phys_addr pageRoot;

        Pagemap(phys_addr pageRoot);

        /**
         * Allocates enough pages to fit the specified size and zeroes them out.
         * @param bytes Requested size in bytes.
         * @returns Virtual address or nullptr on failure.
         */
        void* alloc(size_t bytes) {
            return alloc(bytes, 0);
        }

        /**
         * Allocates enough pages to fit the specified size and zeroes them out.
         * @param bytes Requested size in bytes.
         * @param flags Optional flags.
         * @returns Virtual address or nullptr on failure.
         */
        void* alloc(size_t bytes, uint32_t flags);

        /**
         * Allocates enough physically contiguous pages to fit the specified size and
         * zeroes them out.
         * @param bytes Requested size in bytes.
         * @returns Virtual address or nullptr on failure.
         */
        void* allocContinuous(size_t bytes) {
            return allocContinuous(bytes, 0);
        }

        /**
         * Allocates enough physically contiguous pages to fit the specified size and
         * zeroes them out.
         * @param bytes Requested size in bytes.
         * @param flags Optional flags.
         * @returns Virtual address or nullptr on failure.
         */
        void* allocContinuous(size_t bytes, uint32_t flags);

        /**
         * Maps the specified size to a physical address.
         * @param bytes Requested size in bytes.
         * @param paddr The physical address.
         * @param flags Flags.
         * @returns Virtual address or nullptr on failure.
         */
        void* map(size_t bytes, phys_addr paddr, uint16_t flags);

        /**
         * Gets the physical address of a virtual address. Respects non-page-aligned addresses.
         * @param vaddr The virtual address.
         * @returns The physical address or 0 on failure.
         */
        phys_addr paddrof(void* vaddr);

      private:
        Spinlock spinlock;

        void assign(int pptr, void* virt, phys_addr phys, uint16_t flags);

        uint64_t* findTable(int pptr, uint64_t virt_addr) {
            uint64_t skip_by;
            return findTable(pptr, virt_addr, &skip_by);
        }
        uint64_t* findTable(int pptr, uint64_t virt_addr, uint64_t* skip_by);

        uint64_t* findTableEnsure(int pptr, uint64_t virt_addr);

        void* findContiguous(int pptr, size_t amount);
    };

    /**
     * The pagemap that is used by the kernel exclusively.
     */
    extern Pagemap* kernel_pagemap;

    void init();

    void cleanup_bootstrap();
}