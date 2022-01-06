#include "aex/mem/heap.hpp"

#include "aex/arch/sys/cpu.hpp"
#include "aex/debug.hpp"
#include "aex/kpanic.hpp"
#include "aex/math.hpp"
#include "aex/mem.hpp"
#include "aex/printk.hpp"
#include "aex/spinlock.hpp"
#include "aex/string.hpp"

#include "mem/heap.hpp"

#include <stddef.h>
#include <stdint.h>

constexpr auto ALLOC_SIZE = 16;
constexpr auto SANITY_XOR = 0x28AC829B1F5231EC;

// Idea: Make heap hybrid - use the heap itself for smaller things and paging for larger things
// Idea: Make realloc just unmark the pieces instead of reallocing when shrinking

namespace AEX::Mem::Heap {
    template <typename T>
    T ceilToAllocSize(T val) {
        return int_ceil<T>(val, ALLOC_SIZE);
    }

    template <typename T>
    T ceilToPiece(T val) {
        return int_ceil<T>(val, ALLOC_SIZE) / ALLOC_SIZE;
    }

    typedef uint32_t bitmap_t;

    uint64_t heap_allocated = 0;
    uint64_t heap_free      = 0;

    class Slab {
        public:
        size_t pieces;
        size_t free_pieces;

        Slab*    next;
        Spinlock lock;

        Slab(size_t size) {
            pieces      = size / ALLOC_SIZE;
            free_pieces = pieces;
        }

        static auto createFromVMem(size_t size) {
            size_t pieces      = int_floor<size_t>(size / ALLOC_SIZE, sizeof(bitmap_t) * 8);
            size_t data_offset = ceilToAllocSize(sizeof(Slab) + ceilToAllocSize(pieces / 8));

            size += data_offset;

            void* mem  = Mem::kernel_pagemap->alloc(size, PAGE_WRITE);
            auto  slab = (Slab*) mem;

            slab->pieces      = pieces;
            slab->free_pieces = pieces;
            slab->data        = (void*) ((size_t) mem + data_offset);
            slab->next        = nullptr;

            memset(slab->bitmap, '\0', pieces);

            Mem::atomic_add(&heap_allocated, pieces * ALLOC_SIZE);
            Mem::atomic_add(&heap_free, pieces * ALLOC_SIZE);

            return slab;
        }

#pragma GCC optimize("O2")

        void* alloc(size_t size) {
            if (size == 0)
                return nullptr;

            size += ALLOC_SIZE;

            lock.acquire();

            size_t  pieces = ceilToPiece(size);
            int64_t start  = findFree(pieces);
            if (start == -1) {
                lock.release();
                return nullptr;
            }

            mark(start, pieces);
            lock.release();

            void* addr = (void*) ((size_t) data + start * ALLOC_SIZE);

            memset(addr, '\0', pieces * ALLOC_SIZE);

            auto header    = (alloc_block*) addr;
            header->len    = pieces;
            header->sanity = ((size_t) addr + ALLOC_SIZE) ^ SANITY_XOR;

            Mem::atomic_sub(&heap_free, (uint64_t) pieces * ALLOC_SIZE);

            return (void*) ((size_t) addr + ALLOC_SIZE);
        }

        void free(void* ptr) {
            void* block  = (void*) ((size_t) ptr - ALLOC_SIZE);
            auto  header = (alloc_block*) block;

            if (header->sanity != ((size_t) ptr ^ SANITY_XOR)) {
                printk_fault();

                Debug::dump_bytes((uint8_t*) header - 96, 128 + 80);

                kpanic("heap: free(%p) sanity check failed (sanity was 0x%lx (0x%lx), should "
                       "have been %p)",
                       ptr, header->sanity, header->sanity ^ SANITY_XOR, (size_t) ptr ^ SANITY_XOR);
            }

            using(lock) {
                unmark((uint32_t) ((size_t) block - (size_t) data) / ALLOC_SIZE, header->len);
                Mem::atomic_add(&heap_free, (uint64_t) header->len * ALLOC_SIZE);

                memset(block, '\x5A', header->len * ALLOC_SIZE);
            }
        }

        bool owns(void* ptr) {
            return ptr >= data && ptr <= (void*) ((size_t) data + pieces * ALLOC_SIZE);
        }

        size_t size(void* ptr) {
            auto header = (alloc_block*) ((size_t) ptr - ALLOC_SIZE);

            if (header->sanity != ((size_t) ptr ^ SANITY_XOR)) {
                printk_fault();

                Debug::dump_bytes((uint8_t*) header - 96, 128 + 80);

                kpanic("heap: size(%p) sanity check failed (sanity was 0x%lx (0x%lx), should "
                       "have been %p)",
                       ptr, header->sanity, header->sanity ^ SANITY_XOR, (size_t) ptr ^ SANITY_XOR);
            }

            return (header->len - 1) * ALLOC_SIZE;
        }

        private:
        struct alloc_block {
            size_t sanity;

            uint32_t len; // This is the piece count including this header
            bool     page;
        };

        static_assert(sizeof(alloc_block) <= ALLOC_SIZE);

        void*    data;
        bitmap_t bitmap[];

        void mark(uint32_t start, uint32_t amount) {
            if (amount == 0)
                return;

            uint32_t ii = start / (sizeof(bitmap_t) * 8);
            uint32_t ib = start % (sizeof(bitmap_t) * 8);

            bitmap_t buffer = bitmap[ii];

            while (amount > 0) {
                buffer |= 1 << ib;

                ib++;

                if (ib >= sizeof(bitmap_t) * 8) {
                    bitmap[ii] = buffer;

                    ii++;
                    ib = 0;

                    buffer = bitmap[ii];
                }

                amount--;
            }

            bitmap[ii] = buffer;
        }

        void unmark(uint32_t start, uint32_t amount) {
            if (amount == 0)
                return;

            uint32_t ii = start / (sizeof(bitmap_t) * 8);
            uint32_t ib = start % (sizeof(bitmap_t) * 8);

            bitmap_t buffer = bitmap[ii];

            while (amount > 0) {
                buffer &= ~(1 << ib);

                ib++;

                if (ib >= sizeof(bitmap_t) * 8) {
                    bitmap[ii] = buffer;

                    ii++;
                    ib = 0;

                    buffer = bitmap[ii];
                }

                amount--;
            }

            bitmap[ii] = buffer;
        }

        int64_t findFree(uint32_t amount) {
            if (amount > free_pieces || amount == 0)
                return -1;

            uint32_t index = 0;
            uint32_t ii    = 0;

            uint32_t combo = 0;
            int64_t  start = -1;

            bitmap_t buffer;

            while (index <= pieces) {
                buffer = bitmap[ii];
                if (buffer == (bitmap_t) -1) {
                    index += sizeof(bitmap_t) * 8;

                    start = -1;
                    combo = 0;

                    ii++;
                    continue;
                }

                size_t left = min<size_t>(pieces - index, sizeof(bitmap_t) * 8);

                for (size_t i = 0; i < left; i++) {
                    if (!(buffer & (1 << i))) {
                        combo++;

                        if (start == -1)
                            start = index;

                        if (combo == amount)
                            return start;
                    }
                    else {
                        start = -1;
                        combo = 0;
                    }

                    index++;
                }

                ii++;
            }

            return -1;
        }

        friend void free(void*);
    };

    Slab* rootSlab;

    void* malloc(size_t size) {
        static Spinlock malloc_lock;

        if (size == 0)
            return nullptr;

        if (size > 67108864) {
            printk(WARN "heap: Tried to malloc(%li)\n", size);
            return nullptr;
        }

        auto slab = rootSlab;

        do {
            void* addr = slab->alloc(size);
            if (addr != nullptr)
                return addr;

            if (!slab->next) {
                SCOPE(malloc_lock);

                if (slab->next)
                    continue;

                slab->next = Slab::createFromVMem(max<size_t>(size * 2, 0x100000));
            }

            slab = slab->next;
        } while (slab != nullptr);

        kpanic("heap: malloc(%li) failed [t%li, f%li]", size, heap_allocated, heap_free);
    }

    void free(void* ptr) {
        if (!ptr)
            return;

        auto slab = rootSlab;

        do {
            if (slab->owns(ptr)) {
                slab->free(ptr);
                return;
            }

            slab = slab->next;
        } while (slab != nullptr);

        slab = rootSlab;

        do {
            printk("mmm: %p-%p\n", slab->data, (size_t) slab->data + slab->pieces * ALLOC_SIZE);
            slab = slab->next;
        } while (slab != nullptr);

        kpanic("heap: free(%p) failed", ptr);
    }

    size_t msize(void* ptr) {
        if (!ptr)
            return 0;

        auto slab = rootSlab;

        do {
            if (slab->owns(ptr))
                return slab->size(ptr);

            slab = slab->next;
        } while (slab != nullptr);

        kpanic("AEX::Heap::msize() failed");

        return 0;
    }

    size_t msize_total(void* ptr) {
        return msize(ptr) + ALLOC_SIZE;
    }

    size_t msize_total(size_t len) {
        return int_ceil(len, (size_t) ALLOC_SIZE) + ALLOC_SIZE;
    }

    void* realloc(void* ptr, size_t size) {
        if (!ptr)
            return malloc(size);

        if (size == 0) {
            free(ptr);
            return nullptr;
        }

        size_t oldsize = msize(ptr);
        void*  newptr  = malloc(size);

        if (!newptr) {
            free(ptr);
            return nullptr;
        }

        memcpy(newptr, ptr, min(oldsize, size));
        free(ptr);

        return newptr;
    }

    void init() {
        rootSlab = Slab::createFromVMem(0x100000);

        void* a = malloc(24);
        void* b = malloc(24);

        free(a);

        void* c = malloc(24);

        if (a != c)
            kpanic("AEX::Heap is braindead");

        free(b);
        free(c);
    }
}