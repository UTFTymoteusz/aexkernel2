#include "aex/mem/phys.hpp"

#include "aex/arch/sys/cpu.hpp"
#include "aex/kpanic.hpp"
#include "aex/math.hpp"
#include "aex/mem.hpp"
#include "aex/printk.hpp"
#include "aex/spinlock.hpp"
#include "aex/string.hpp"

#include "boot/mboot.h"
#include "mem/frame_piece.hpp"
#include "mem/phys.hpp"
#include "mem/sections.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX::Mem::Phys {
    typedef size_t phys_t;

    uint8_t      root_piece_memory[8192];
    frame_piece* first_piece;
    frame_piece* current_piece;
    frame_piece* prev_piece = nullptr;

    Spinlock lock;

    size_t frames_available;
    size_t frames_taken_by_kernel;

    static void _createPieces(size_t addr, uint32_t frames) {
        if (current_piece == first_piece) {
            int amnt = min<int>((sizeof(root_piece_memory) - sizeof(frame_piece)) * 8, frames);
            frames -= amnt;

            first_piece->init((phys_t) addr, amnt);

            frames_taken_by_kernel = ceiltopg((uint64_t) &_end_bss - (uint64_t) &_start_text);

            first_piece->alloc(0, frames_taken_by_kernel);

            addr += amnt * Sys::CPU::PAGE_SIZE;
        }

        if (frames == 0)
            return;

        int frp_size = ceiltopg(sizeof(frame_piece) + (frames + 1) / 8) * Sys::CPU::PAGE_SIZE;

        frame_piece* new_piece = (frame_piece*) (alloc(frp_size) + (size_t) &KERNEL_EXEC_VMA);

        new_piece->init((phys_t) addr, frames);

        current_piece->next = new_piece;
        current_piece       = new_piece;
    }

    void init(const multiboot_info_t* mbinfo) {
        printk(INIT "Enumerating memory\n");

        auto*    mmap = (multiboot_memory_map_t*) (size_t) mbinfo->mmap_addr;
        uint32_t amnt = mbinfo->mmap_length / sizeof(multiboot_memory_map_t);

        first_piece   = (frame_piece*) &root_piece_memory;
        current_piece = first_piece;

        for (size_t i = 0; i < amnt; i++) {
            auto* mmap_entry = &mmap[i];

            if (mmap_entry->type != 1 || mmap_entry->addr < 0x100000)
                continue;

            if (mmap_entry->addr & 0xFFF)
                kpanic("Misaligned memory region encountered");

            int64_t frames = mmap_entry->len / Sys::CPU::PAGE_SIZE;
            if (frames < 8)
                continue;

            frames_available += frames;

            printk("0x%016p, %li (%li MiB)\n", mmap_entry->addr, frames * Sys::CPU::PAGE_SIZE,
                   (frames * Sys::CPU::PAGE_SIZE) / 1048576);

            _createPieces(mmap_entry->addr, frames);
        }

        int          pieceCount = 0;
        frame_piece* piece      = first_piece;

        do {
            pieceCount++;
            piece = piece->next;
        } while (piece != nullptr);

        printk("Available memory: %i KiB\n", frames_available * Sys::CPU::PAGE_SIZE / 1024);
        printk("Kernel memory   : %i KiB\n", frames_taken_by_kernel * Sys::CPU::PAGE_SIZE / 1024);
        printk("Memory pieces   : %i\n", pieceCount);

        printk(OK "Memory enumerated\n");
    }

    phys_t alloc(int32_t amount) {
        uint32_t     frames = ceiltopg(amount);
        frame_piece* piece  = first_piece;

        SCOPE(lock);

        do {
            if (piece->frames_free < frames)
                continue;

            int32_t start = piece->find(frames);
            if (start == -1)
                continue;

            frames_available -= frames;
            piece->alloc(start, frames);

            return piece->start + start * Sys::CPU::PAGE_SIZE;
        } while ((piece = piece->next) != nullptr);

        lock.release();
        kpanic("Failed to AEX::Mem::Phys::alloc()");
    }

    void free(phys_t addr, int32_t amount) {
        if (addr < first_piece->start)
            return;

        uint32_t     frames = ceiltopg(amount);
        frame_piece* piece  = first_piece;

        SCOPE(lock);

        do {
            if (piece->start > addr)
                continue;

            addr -= piece->start;
            frames_available += frames;

            piece->free(addr / Sys::CPU::PAGE_SIZE, frames);
            return;
        } while ((piece = piece->next) != nullptr);

        lock.release();
        kpanic("Failed to AEX::Mem::Phys::free()");
    }

    bool mask(phys_t addr, int32_t amount) {
        if (addr < first_piece->start)
            return false;

        uint32_t     frames = ceiltopg(amount);
        frame_piece* piece  = first_piece;

        SCOPE(lock);

        do {
            if (piece->start > addr)
                continue;

            addr -= piece->start;

            piece->alloc(addr / Sys::CPU::PAGE_SIZE, frames);
            return true;
        } while ((piece = piece->next) != nullptr);

        return false;
    }
}