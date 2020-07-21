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

        asm volatile("mov %0, rbp;" : "=r"(frame));

        while (frame > (stack_frame*) 8) {
            if (skip != 0) {
                skip--;
                continue;
            }

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
                if (frame->rip == (size_t) Proc::Thread::exit)
                    printk("  *thread entry/exit*\n");
                else {
                    int         delta = 0;
                    const char* name  = symbol_addr2name((void*) frame->rip, &delta);

                    printk("  0x%p <%s+0x%x>\n", frame->rip, name ? name : "no idea", delta);
                }

                break;
            }

            frame = frame->rbp;
        }
    }
}