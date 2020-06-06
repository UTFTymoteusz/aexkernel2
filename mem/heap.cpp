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

#include <stdint.h>

#define ALLOC_SIZE 16

// Idea: Make heap hybrid - use the heap itself for smaller things and paging for larger things

namespace AEX::Heap {
    template <typename T>
    T ceilToAllocSize(T val) {
        return int_ceil<T>(val, ALLOC_SIZE);
    }

    template <typename T>
    T ceilToPiece(T val) {
        return (val + ALLOC_SIZE - 1) / ALLOC_SIZE;
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
            size_t pieces      = int_floor<size_t>(size / ALLOC_SIZE, sizeof(uint32_t) * 8);
            size_t data_offset = ceilToAllocSize(sizeof(Piece) + ceilToAllocSize(pieces / 8));

            size += data_offset;

            void* mem   = AEX::VMem::kernel_pagemap->alloc(size, PAGE_WRITE);
            auto  piece = (Piece*) mem;

            piece->pieces      = pieces;
            piece->free_pieces = pieces;
            piece->data        = (void*) ((size_t) mem + data_offset);
            piece->next        = nullptr;

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

            memset((void*) ((size_t) data + start * ALLOC_SIZE), '\0', pieces * ALLOC_SIZE);

            auto header = (alloc_block*) ((size_t) data + start * ALLOC_SIZE);
            header->len = pieces;

            Mem::atomic_sub(&heap_free, (uint64_t) pieces * ALLOC_SIZE);

            return (void*) ((size_t) data + start * ALLOC_SIZE + ALLOC_SIZE);
        }

        void free(void* ptr) {
            void* block  = (void*) ((size_t) ptr - ALLOC_SIZE);
            auto  header = (alloc_block*) block;

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
            return header->len * ALLOC_SIZE;
        }

      private:
        struct alloc_block {
            uint32_t len; // This is the piece count including this header
        } __attribute((packed));

        void*    data;
        uint32_t bitmap[];

        void mark(uint32_t start, uint32_t amount) {
            if (amount == 0)
                return;

            uint32_t ii = start / (sizeof(uint32_t) * 8);
            uint32_t ib = start % (sizeof(uint32_t) * 8);

            while (amount > 0) {
                bitmap[ii] |= 1 << ib;

                ib++;

                if (ib >= sizeof(uint32_t) * 8) {
                    ii++;
                    ib = 0;
                }

                amount--;
            }
        }

        void unmark(uint32_t start, uint32_t amount) {
            if (amount == 0)
                return;

            uint32_t ii = start / (sizeof(uint32_t) * 8);
            uint32_t ib = start % (sizeof(uint32_t) * 8);

            while (amount > 0) {
                bitmap[ii] &= ~(1 << ib);

                ib++;

                if (ib >= sizeof(uint32_t) * 8) {
                    ii++;
                    ib = 0;
                }

                amount--;
            }
        }

        int64_t findFree(uint32_t amount) {
            if (amount > free_pieces || amount == 0)
                return -1;

            uint32_t index = 0;
            uint32_t ii    = 0;

            uint32_t combo = 0;
            int64_t  start = -1;

            uint32_t buffer;

            while (index <= pieces) {
                buffer = bitmap[ii];
                if (buffer == 0xFFFFFFFF) {
                    index += sizeof(uint32_t) * 8;

                    ii++;
                    continue;
                }

                size_t left = min<size_t>(pieces - index, sizeof(uint32_t) * 8);

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

        void*  newptr  = malloc(size);
        size_t oldsize = msize(ptr);

        if (newptr && ptr)
            memcpy(newptr, ptr, oldsize);

        if (oldsize < size && newptr)
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