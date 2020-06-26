#include "aex/mem/heap.hpp"

#include "aex/arch/sys/cpu.hpp"
#include "aex/debug.hpp"
#include "aex/kpanic.hpp"
#include "aex/math.hpp"
#include "aex/mem/atomic.hpp"
#include "aex/mem/vmem.hpp"
#include "aex/printk.hpp"
#include "aex/spinlock.hpp"
#include "aex/string.hpp"

#include <stddef.h>
#include <stdint.h>

#define BITMAP_TYPE uint32_t
#define ALLOC_SIZE 16
#define SANITY_XOR 0x28AC829B1F5231EC

// Idea: Make heap hybrid - use the heap itself for smaller things and paging for larger things

namespace AEX::Heap {
    template <typename T>
    T ceilToAllocSize(T val) {
        return int_ceil<T>(val, ALLOC_SIZE);
    }

    template <typename T>
    T ceilToPiece(T val) {
        return int_ceil<T>(val, ALLOC_SIZE) / ALLOC_SIZE;
    }

    uint64_t heap_allocated = 0;
    uint64_t heap_free      = 0;

    class Piece {
        public:
        size_t pieces;
        size_t free_pieces;

        Piece*   next;
        Spinlock spinlock;

        Piece(size_t size) {
            pieces      = size / ALLOC_SIZE;
            free_pieces = pieces;
        }

        static Piece* createFromVMem(size_t size) {
            size_t pieces      = int_floor<size_t>(size / ALLOC_SIZE, sizeof(BITMAP_TYPE) * 8);
            size_t data_offset = ceilToAllocSize(sizeof(Piece) + ceilToAllocSize(pieces / 8));

            size += data_offset;

            void* mem   = AEX::VMem::kernel_pagemap->alloc(size, PAGE_WRITE);
            auto  piece = (Piece*) mem;

            piece->pieces      = pieces;
            piece->free_pieces = pieces;
            piece->data        = (void*) ((size_t) mem + data_offset);
            piece->next        = nullptr;

            memset(piece->bitmap, '\0', pieces);

            Mem::atomic_add(&heap_allocated, pieces * ALLOC_SIZE);
            Mem::atomic_add(&heap_free, pieces * ALLOC_SIZE);

            return piece;
        }

        void* alloc(size_t size) {
            if (size == 0)
                return nullptr;

            size += ALLOC_SIZE;

            spinlock.acquire();

            size_t  pieces = ceilToPiece(size);
            int64_t start  = findFree(pieces);
            if (start == -1) {
                spinlock.release();
                return nullptr;
            }

            mark(start, pieces);
            spinlock.release();

            void* addr = (void*) ((size_t) data + start * ALLOC_SIZE);

            memset(addr, '\0', pieces * ALLOC_SIZE);

            auto header         = (alloc_block*) addr;
            header->len         = pieces;
            header->sanity      = ((size_t) addr + ALLOC_SIZE) ^ SANITY_XOR;
            header->recognition = 0x55AA;

            Mem::atomic_sub(&heap_free, (uint64_t) pieces * ALLOC_SIZE);

            return (void*) ((size_t) addr + ALLOC_SIZE);
        }

        void free(void* ptr) {
            void* block  = (void*) ((size_t) ptr - ALLOC_SIZE);
            auto  header = (alloc_block*) block;

            if (header->sanity != ((size_t) ptr ^ SANITY_XOR)) {
                printk_fault();

                Debug::dump_bytes((uint8_t*) header - 96, 128 + 80);

                kpanic("heap: free(0x%p) sanity check failed (sanity was 0x%lx (0x%lx), should "
                       "have been %p)",
                       ptr, header->sanity, header->sanity ^ SANITY_XOR, (size_t) ptr ^ SANITY_XOR);
            }

            spinlock.acquire();
            unmark((uint32_t)((size_t) block - (size_t) data) / ALLOC_SIZE, header->len);
            Mem::atomic_add(&heap_free, (uint64_t) header->len * ALLOC_SIZE);

            memset(block, '\0', header->len * ALLOC_SIZE);
            spinlock.release();
        }

        bool owns(void* ptr) {
            return ptr >= data && ptr <= (void*) ((size_t) data + pieces * ALLOC_SIZE);
        }

        size_t getAllocSize(void* ptr) {
            auto header = (alloc_block*) ((size_t) ptr - ALLOC_SIZE);
            return (header->len - 1) * ALLOC_SIZE;
        }

        private:
        struct alloc_block {
            size_t sanity;

            uint32_t len; // This is the piece count including this header
            bool     page;
            uint16_t recognition;
        };

        static_assert(sizeof(alloc_block) <= ALLOC_SIZE);

        void*       data;
        BITMAP_TYPE bitmap[];

        void mark(uint32_t start, uint32_t amount) {
            if (amount == 0)
                return;

            uint32_t ii = start / (sizeof(BITMAP_TYPE) * 8);
            uint32_t ib = start % (sizeof(BITMAP_TYPE) * 8);

            BITMAP_TYPE buffer = bitmap[ii];

            while (amount > 0) {
                buffer |= 1 << ib;

                ib++;

                if (ib >= sizeof(BITMAP_TYPE) * 8) {
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

            uint32_t ii = start / (sizeof(BITMAP_TYPE) * 8);
            uint32_t ib = start % (sizeof(BITMAP_TYPE) * 8);

            BITMAP_TYPE buffer = bitmap[ii];

            while (amount > 0) {
                buffer &= ~(1 << ib);

                ib++;

                if (ib >= sizeof(BITMAP_TYPE) * 8) {
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

            BITMAP_TYPE buffer;

            while (index <= pieces) {
                buffer = bitmap[ii];
                if (buffer == (BITMAP_TYPE) -1) {
                    index += sizeof(BITMAP_TYPE) * 8;

                    start = -1;
                    combo = 0;

                    ii++;
                    continue;
                }

                size_t left = min<size_t>(pieces - index, sizeof(BITMAP_TYPE) * 8);

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

    Piece* rootPiece;

    void* malloc(size_t size) {
        static Spinlock malloc_lock;

        if (size == 0)
            return nullptr;

        if (size > 67108864) {
            printk(PRINTK_WARN "heap: Tried to malloc(%li)\n", size);
            return nullptr;
        }

        Piece* piece = rootPiece;

        do {
            void* addr = piece->alloc(size);
            if (addr != nullptr)
                return addr;

            if (!piece->next) {
                malloc_lock.acquire();

                if (piece->next) {
                    malloc_lock.release();
                    continue;
                }

                piece->next = Piece::createFromVMem(max<size_t>(size * 2, 0x100000));
                malloc_lock.release();
            }

            piece = piece->next;
        } while (piece != nullptr);

        kpanic("heap: malloc(%li) failed [t%li, f%li]\n", size, heap_allocated, heap_free);
    }

    void free(void* ptr) {
        if (!ptr)
            return;

        Piece* piece = rootPiece;

        do {
            if (piece->owns(ptr)) {
                piece->free(ptr);
                return;
            }

            piece = piece->next;
        } while (piece != nullptr);

        piece = rootPiece;

        do {
            printk("mmm: 0x%p-0x%p\n", piece->data,
                   (size_t) piece->data + piece->pieces * ALLOC_SIZE);
            piece = piece->next;
        } while (piece != nullptr);

        kpanic("heap: free(0x%p) failed", ptr);
    }

    size_t msize(void* ptr) {
        if (!ptr)
            return 0;

        Piece* piece = rootPiece;

        do {
            if (piece->owns(ptr))
                return piece->getAllocSize(ptr);

            piece = piece->next;
        } while (piece != nullptr);

        kpanic("AEX::Heap::msize() failed");

        return 0;
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

        if (!newptr)
            return nullptr;

        memcpy(newptr, ptr, min(oldsize, size));

        if (oldsize < size)
            memset((void*) ((size_t) newptr + oldsize), '\0', size - oldsize);

        free(ptr);

        return newptr;
    }

    void init() {
        rootPiece = Piece::createFromVMem(0x100000);

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