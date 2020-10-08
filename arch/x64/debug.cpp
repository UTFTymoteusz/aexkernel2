#include "aex/debug.hpp"

#include "aex/printk.hpp"
#include "aex/proc.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX::Debug {
    struct stack_frame {
        stack_frame* rbp;
        uint64_t     rip;
    };

    void stack_trace(int skip) {
        stack_frame* frame;

        const int max   = 12;
        int       level = 0;

        asm volatile("mov %0, rbp;" : "=r"(frame));

        while (frame > (stack_frame*) 8) {
            if (skip != 0) {
                skip--;
                continue;
            }

            level++;

            if (level > max)
                return;

            switch (frame->rip) {
            case ENTRY_BOOT:
                printk("  *bootstrap entry*\n");
                return;
            case ENTRY_USER:
                printk("  *user entry*\n");
                return;
            case ENTRY_KERNEL:
                printk("  *kernel entry*\n");
                return;
            default:
                if (frame->rip == (size_t) Proc::Thread::exit) {
                    printk("  *thread entry/exit*\n");
                    return;
                }

                int         delta = 0;
                const char* name  = addr2name((void*) frame->rip, delta);

                printk("  0x%p <%s+0x%x>\n", frame->rip, name ? name : "no idea", delta);

                // if ((frame->rip & 0xFFFFFFFFF0000000) != 0xFFFFFFFF80000000)
                //    return;

                break;
            }

            frame = frame->rbp;
            // if (((size_t) frame & 0xFFFF000000000000) != 0xFFFF000000000000)
            //    return;
        }
    }
}