#pragma once

#include "aex/arch/sys/cpu.hpp"

#include <stddef.h>
#include <stdint.h>

struct multiboot_info;
typedef struct multiboot_info multiboot_info_t;

namespace AEX::PMem {
    typedef size_t phys_addr;

    extern size_t frames_available;
    extern size_t frames_taken_by_kernel;

    /**
     * Initializes the memory frame allocator.
     * @param mbinfo The multiboot info struct.
     */
    void init(const multiboot_info_t* mbinfo);

    /**
     * Ceils the amount of bytes to the required amount of frames.
     */
    template <typename T>
    T ceiltopg(T a) {
        return (a + (Sys::CPU::PAGE_SIZE - 1)) / Sys::CPU::PAGE_SIZE;
    }

    /**
     * Allocates enough frames to fit the specified size.
     * @param amount Requested size in bytes.
     * @returns The physical address of the allocated frame region.
     */
    phys_addr alloc(int32_t amount);

    /**
     * Frees the amount of frames required to fit the specified size starting at the specified
     * address.
     * @param addr   The physical address.
     * @param amount Size in bytes.
     */
    void free(phys_addr addr, int32_t amount);
}