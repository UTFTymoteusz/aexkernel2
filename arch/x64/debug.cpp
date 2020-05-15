#include "aex/debug.hpp"

#include "aex/printk.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX::Debug {
    struct stack_frame {
        stack_frame* rbp;
        uint64_t     rip;
    };

    void stack_trace(int skip) {
        stack_frame* frame;

        asm volatile("mov %0, rbp;" : "=r"(frame));

        while (frame > (stack_frame*) 8) {
            if (skip == 0)
                switch (frame->rip) {
                case entry_type::BOOT:
                    printk("  *bootstrap entry*\n");
                    return;
                case entry_type::USER:
                    printk("  *user entry*\n");
                    return;
                case entry_type::KERNEL:
                    printk("  *kernel entry*\n");
                    return;
                default:
                    printk("  0x%p\n", frame->rip);
                    break;
                }
            else
                skip--;

            frame = frame->rbp;
        }
    }
}