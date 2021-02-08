#include "aex/dev/tty.hpp"

#include "aex/arch/sys/cpu.hpp"
#include "aex/dev/tty/tty.hpp"
#include "aex/dev/tty/vtty.hpp"
#include "aex/mem.hpp"
#include "aex/printk.hpp"
#include "aex/string.hpp"

#include "boot/mboot.h"
#include "dev/tty/abstractor.hpp"
#include "dev/tty/grtty.hpp"
#include "dev/tty/txtty.hpp"

#include <stddef.h>
#include <stdint.h>

using namespace AEX::Dev::Input;

namespace AEX::Dev::TTY {
    VTTY*      VTTYs[TTY_AMOUNT];
    TTY*       TTYs[TTY_AMOUNT];
    TxTTY      tx_init_vtty;
    GrTTY      gr_init_vtty;
    Abstractor init_tty;

    uint16_t buffer[80 * 25];

    void init(multiboot_info_t* mbinfo) {
        tx_init_vtty.clear();

        if ((mbinfo->flags & (1 << 2)) &&
            mbinfo->framebuffer_type == MULTIBOOT_FRAMEBUFFER_TYPE_EGA_TEXT) {
            new (&tx_init_vtty) TxTTY((void*) 0xFFFFFFFF800B8000);
        }
        else {
            memset64(buffer, '\0', sizeof(buffer) / sizeof(uint64_t));
            new (&tx_init_vtty) TxTTY((void*) &buffer);
        }

        VTTYs[0] = &tx_init_vtty;

        new (&init_tty) Abstractor(VTTYs[0]);
        TTYs[0] = &init_tty;
    }

    void init_mem(multiboot_info_t* mbinfo) {
        if ((mbinfo->flags & (1 << 2)) &&
            mbinfo->framebuffer_type == MULTIBOOT_FRAMEBUFFER_TYPE_EGA_TEXT) {
            uint32_t* mapped = (uint32_t*) Mem::kernel_pagemap->map(
                2 * 80 * 25, mbinfo->framebuffer_addr, PAGE_COMBINE | PAGE_WRITE);

            for (int i = 1; i < TTY_AMOUNT; i++) {
                VTTYs[i] = new TxTTY((void*) mapped);
                TTYs[i]  = new Abstractor(VTTYs[i]);
            }

            tx_init_vtty.remap(mapped);
            return;
        }

        new (&gr_init_vtty) GrTTY(mbinfo);

        gr_init_vtty.fromVGA((GrTTY::vga_char*) tx_init_vtty.vga_output());
        VTTYs[0] = &gr_init_vtty;

        gr_init_vtty.setCursorX(tx_init_vtty.getCursorX());
        gr_init_vtty.setCursorY(tx_init_vtty.getCursorY());

        new (&init_tty) Abstractor(VTTYs[0]);

        for (int i = 1; i < TTY_AMOUNT; i++) {
            VTTYs[i] = new GrTTY(mbinfo);
            TTYs[i]  = new Abstractor(VTTYs[i]);
        }
    }
}