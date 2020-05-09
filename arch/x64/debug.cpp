#include "aex/debug.hpp"

#include "aex/printk.hpp"

#include <stdint.h>

namespace AEX::Debug {
    struct stack_frame {
        stack_frame* rbp;
        uint64_t     rip;
    };

    void stack_trace(int skip) {
        stack_frame* frame;

        asm volatile("mov %0, rbp;" : "=r"(frame));

        while (frame) {
            if (skip == 0)
                printk("  0x%p\n", frame->rip);
            else
                skip--;

            frame = frame->rbp;
        }
    }
}