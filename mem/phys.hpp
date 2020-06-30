#pragma once

namespace AEX::Mem::Phys {
    /**
     * Initializes the memory frame allocator.
     * @param mbinfo The multiboot info struct.
     */
    void init(const multiboot_info_t* mbinfo);
}