#include "mem/pmem.hpp"

#include "kernel/kpanic.hpp"
#include "kernel/math.hpp"
#include "kernel/printk.hpp"
#include "kernel/spinlock.hpp"
#include "kernel/string.hpp"
#include "mem/memory.hpp"
#include "sys/cpu.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX::PMem {
    class FramePiece {
      public:
        phys_addr start;

        uint32_t    size;
        uint32_t    frames_free;
        FramePiece* next;

        FramePiece(phys_addr addr, uint32_t amnt) { init(addr, amnt); }

        void init(phys_addr addr, uint32_t amnt) {
            start = addr;

            size        = amnt;
            frames_free = amnt;

            memset(_bitmap, 0, amnt / 8);
        }

        void alloc(int32_t lid, uint32_t amount) {
            uint32_t ii = lid / (sizeof(uint32_t) * 8);
            uint16_t ib = lid % (sizeof(uint32_t) * 8);

            uint32_t tmp;

            for (size_t i = 0; i < amount; i++) {
                tmp = 1 << ib;

                if (_bitmap[ii] & tmp)
                    kpanic("FramePiece::Alloc(%i, %u) tried to alloc an "
                           "alloced frame.",
                           lid, amount);

                _bitmap[ii] |= tmp;

                ib++;
                if (ib >= sizeof(uint32_t) * 8) {
                    ii++;
                    ib = 0;
                }
            }
            frames_free -= amount;
        }

        void free(int32_t lid, uint32_t amount) {
            uint32_t ii = lid / (sizeof(uint32_t) * 8);
            uint16_t ib = lid % (sizeof(uint32_t) * 8);

            uint32_t tmp;

            for (size_t i = 0; i < amount; i++) {
                tmp = 1 << ib;

                if (!(_bitmap[ii] & tmp))
                    kpanic("FramePiece::Free(%i, %u) tried to free a unalloced "
                           "frame.",
                           lid, amount);

                _bitmap[ii] = _bitmap[ii] & ~tmp;

                ib++;
                if (ib >= sizeof(uint32_t) * 8) {
                    ii++;
                    ib = 0;
                }
            }
            frames_free += amount;
        }

        int32_t find(uint32_t amount) {
            if (amount == 0 || amount > frames_free)
                return -1;

            uint32_t ii = 0;
            uint16_t ib = 0;

            uint32_t found = 0;
            int32_t  start = -1;

            uint32_t tmp;

            for (size_t i = 0; i < size; i++) {
                tmp = 1 << ib;

                if (!(_bitmap[ii] & tmp)) {
                    if (start == -1)
                        start = i;

                    found++;
                    if (found == amount)
                        return start;
                }
                else {
                    found = 0;
                    start = -1;
                }

                ib++;
                if (ib >= sizeof(uint32_t) * 8) {
                    ii++;
                    ib = 0;
                }
            }
            return -1;
        }

      private:
        uint32_t _bitmap[];
    };
    typedef size_t phys_addr;

    uint8_t     rootPieceMemory[4096];
    FramePiece* firstPiece = (FramePiece*) rootPieceMemory;
    FramePiece* currentPiece;
    FramePiece* prevPiece = nullptr;

    Spinlock spinlock;

    size_t framesAvailable;
    size_t framesTakenByKernel;

    static void _createPieces(size_t addr, uint32_t frames) {
        if (currentPiece == firstPiece) {
            int amnt = min((4096 - sizeof(FramePiece)) * 8, frames);
            frames -= amnt;

            currentPiece->init((phys_addr) addr, amnt);

            framesTakenByKernel = ceiltopg((uint64_t) &_end_bss - (uint64_t) &_start_text);

            firstPiece->alloc(0, framesTakenByKernel);

            addr += amnt * Sys::CPU::PAGE_SIZE;
        }

        if (frames == 0)
            return;

        int frp_size = ceiltopg(sizeof(FramePiece) + (frames + 1) / 8) * Sys::CPU::PAGE_SIZE;

        FramePiece* new_piece = (FramePiece*) (alloc(frp_size) + (size_t) &KERNEL_VMA);

        new_piece->init((phys_addr) addr, frames);

        currentPiece->next = new_piece;
        currentPiece       = new_piece;
    }

    void init(const multiboot_info_t* mbinfo) {
        printk(PRINTK_INIT "Enumerating memory\n");

        auto*    mmap = (multiboot_memory_map_t*) mbinfo->mmap_addr;
        uint32_t amnt = mbinfo->mmap_length / sizeof(multiboot_memory_map_t);

        currentPiece = firstPiece;

        for (size_t i = 0; i < amnt; i++) {
            auto* mmap_entry = &mmap[i];

            if (mmap_entry->type != 1 || mmap_entry->addr < 0x100000)
                continue;

            phys_addr addr  = mmap_entry->addr;
            phys_addr caddr = ceiltopg(addr);

            int     delta  = caddr - addr;
            int32_t frames = (mmap_entry->len - delta) / Sys::CPU::PAGE_SIZE;

            framesAvailable += frames;

            _createPieces(addr, frames);
        }

        int         pieceCount = 0;
        FramePiece* piece      = firstPiece;

        do {
            pieceCount++;
            piece = piece->next;
        } while (piece != nullptr);

        printk("Available memory: %i KiB\n", framesAvailable * Sys::CPU::PAGE_SIZE / 1024);
        printk("Kernel memory   : %i KiB\n", framesTakenByKernel * Sys::CPU::PAGE_SIZE / 1024);
        printk("Memory pieces   : %i\n", pieceCount);

        printk(PRINTK_OK "Memory enumerated\n");
    }

    phys_addr alloc(int32_t amount) {
        uint32_t    frames = ceiltopg(amount);
        FramePiece* piece  = firstPiece;

        spinlock.acquire();

        do {
            if (piece->frames_free < frames)
                continue;

            int32_t start = piece->find(frames);
            if (start == -1)
                continue;

            piece->alloc(start, frames);
            spinlock.release();

            return piece->start + start * Sys::CPU::PAGE_SIZE;
        } while ((piece = piece->next) != nullptr);

        kpanic("Failed to AEX::PMem::alloc()");
        spinlock.release();

        return 0;
    }

    void free(phys_addr addr, int32_t amount) {
        printk("Frame::free(0x%x, %i)\n", addr, amount);

        if (addr < firstPiece->start)
            return;

        uint32_t    frames = ceiltopg(amount);
        FramePiece* piece  = firstPiece;

        spinlock.acquire();

        do {
            if (addr / Sys::CPU::PAGE_SIZE >= firstPiece->size) {
                addr -= firstPiece->size * Sys::CPU::PAGE_SIZE;
                continue;
            }

            piece->free(addr / Sys::CPU::PAGE_SIZE, frames);
            spinlock.release();

            return;
        } while ((piece = piece->next) != nullptr);

        kpanic("Failed to AEX::PMem::free()");
        spinlock.release();
    }
} // namespace AEX::Frame