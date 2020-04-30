#include "aex/mem/heap.hpp"

#include "aex/kpanic.hpp"
#include "aex/mem/vmem.hpp"
#include "aex/printk.hpp"
#include "aex/spinlock.hpp"
#include "aex/string.hpp"

#include <stdint.h>

#define ALLOC_SIZE 16

namespace AEX::Heap {
    template <typename T>
    T ceilToAllocSize(T val) {
        return (val + ALLOC_SIZE - 1) / ALLOC_SIZE * ALLOC_SIZE;
    }

    template <typename T>
    T ceilToPiece(T val) {
        return (val + ALLOC_SIZE - 1) / ALLOC_SIZE;
    }

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
            size_t pieces      = size / ALLOC_SIZE;
            size_t data_offset = ceilToAllocSize(sizeof(Piece) + ceilToAllocSize(pieces / 8));

            size += data_offset;

            void* mem   = AEX::VMem::kernel_pagemap->alloc(size, PAGE_WRITE);
            auto  piece = (Piece*) mem;

            piece->pieces      = pieces;
            piece->free_pieces = pieces;
            piece->data        = (void*) ((size_t) mem + data_offset);
            piece->next        = nullptr;

            return piece;
        }

        void* alloc(size_t size) {
            size += ALLOC_SIZE;

            spinlock.acquire();

            size_t  pieces = ceilToPiece(size);
            int64_t start  = findFree(pieces);
            if (start == -1)
                return nullptr;

            mark(start, pieces);
            spinlock.release();

            memset((void*) ((size_t) data + start * ALLOC_SIZE), 0, pieces * ALLOC_SIZE);

            auto header = (alloc_block*) ((size_t) data + start * ALLOC_SIZE);
            header->len = pieces;

            return (void*) ((size_t) data + start * ALLOC_SIZE + ALLOC_SIZE);
        }

        void free(void* ptr) {
            void* block  = (void*) ((size_t) ptr - ALLOC_SIZE);
            auto  header = (alloc_block*) block;

            spinlock.acquire();
            unmark((uint32_t)((size_t) block - (size_t) data) / ALLOC_SIZE, header->len);
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

            uint32_t ii = 0;
            uint32_t ib = 0;
            uint32_t tmp;

            uint32_t buffer = bitmap[ii];

            uint32_t combo = 0;
            int64_t  start = -1;

            while (index <= pieces) {
                tmp = 1 << ib;

                if (!(buffer & tmp)) {
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
                ib++;

                if (ib >= sizeof(uint32_t) * 8) {
                    ii++;
                    ib = 0;

                    buffer = bitmap[ii];
                }
            }

            return -1;
        }
    };

    Piece* rootPiece;

    void* malloc(size_t size) {
        Piece* piece = rootPiece;

        do {
            void* addr = piece->alloc(size);
            if (addr != nullptr)
                return addr;

            piece = piece->next;
        } while (piece != nullptr);

        kpanic("AEX::Heap::alloc() failed");
        return nullptr;
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

        kpanic("AEX::Heap::free() failed");
    }

    size_t msize(void* ptr) {
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
        if (ptr == nullptr || size == 0)
            return malloc(size);

        void*  newptr  = malloc(size);
        size_t oldsize = msize(ptr);

        memcpy(newptr, ptr, oldsize);

        return newptr;
    }

    void init() {
        rootPiece = Piece::createFromVMem(0x100000);

        void* a = malloc(24);
        malloc(24);

        free(a);

        void* c = malloc(24);

        if (a != c)
            kpanic("AEX::Heap is braindead");
    }
}