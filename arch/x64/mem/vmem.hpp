#pragma once

#include "kernel/spinlock.hpp"
#include "mem/pmem.hpp"

#include <stdint.h>

extern int _page_present, _page_write, _page_user, _page_nophys;

#define PAGE_PRESENT _page_present
#define PAGE_WRITE _page_write
#define PAGE_USER _page_user
#define PAGE_NOPHYS _page_nophys

namespace AEX::VMem {
    class Pagemap {
      public:
        void* vstart;
        void* vend;

        PMem::phys_addr pageRoot;

        Pagemap(PMem::phys_addr pageRoot);

        /*
         * Allocates enough pages to fit the specified amount of bytes and zeroes them out.
         * Returns a virtual address or nullptr on failure.
         */
        void* alloc(size_t bytes) { return alloc(bytes, 0); }
        void* alloc(size_t bytes, uint32_t flags);

        /*
         * Allocates enough physically contiguous pages to fit the specified amount of bytes and
         * zeros them out. Returns a virtual address or nullptr on failure.
         */
        void* allocContinuous(size_t bytes) { return allocContinuous(bytes, 0); }
        void* allocContinuous(size_t bytes, uint32_t flags);

        /* Maps bytes to the specified physical address. Returns a virtual
         * address or nullptr on failure.
         */
        void* map(size_t bytes, PMem::phys_addr paddr, uint16_t flags);

      private:
        Spinlock spinlock;

        void assign(int pptr, void* virt, PMem::phys_addr phys, uint16_t flags);

        uint64_t* findTable(int pptr, uint64_t virt_addr) {
            uint64_t skip_by;
            return findTable(pptr, virt_addr, &skip_by);
        }
        uint64_t* findTable(int pptr, uint64_t virt_addr, uint64_t* skip_by);

        uint64_t* findTableEnsure(int pptr, uint64_t virt_addr);

        void* findContiguous(int pptr, size_t amount);
    };

    extern Pagemap* kernel_pagemap;

    void init();
}