#include "aex/debug.hpp"

#include "aex/arch/sys/cpu/tss.hpp"
#include "aex/printk.hpp"
#include "aex/proc.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX::Debug {
    struct stack_frame {
        stack_frame* rbp;
        uint64_t     rip;
    };

    bool is_valid(const stack_frame* frame) {
        auto cpu    = Sys::CPU::current();
        auto fstk   = cpu->currentThread()->fault_stack;
        auto kstk   = cpu->currentThread()->kernel_stack;
        auto fstk_p = cpu->previousThread()->fault_stack;
        auto kstk_p = cpu->previousThread()->kernel_stack;

        if (kstk.size == 0 || kstk_p.size == 0) {
            if (!inrange((size_t) frame->rbp, (size_t) fstk.ptr, (size_t) fstk.ptr + fstk.size - 1))
                return false;
        }
        else {
            if (!inrange((size_t) frame->rbp, (size_t) fstk.ptr,
                         (size_t) fstk.ptr + fstk.size - 1) &&
                !inrange((size_t) frame->rbp, (size_t) kstk.ptr,
                         (size_t) kstk.ptr + kstk.size - 1) &&
                !inrange((size_t) frame->rbp, (size_t) fstk_p.ptr,
                         (size_t) fstk_p.ptr + fstk_p.size - 1) &&
                !inrange((size_t) frame->rbp, (size_t) kstk_p.ptr,
                         (size_t) kstk_p.ptr + kstk_p.size - 1) &&
                !inrange((size_t) frame->rbp, cpu->reshed_stack_start,
                         cpu->reshed_stack_start + 4095) &&
                !inrange((size_t) frame->rbp, cpu->irq_stack_start, cpu->irq_stack_start + 16383))
                return false;
        }

        return true;
    }

    void stack_trace(int skip, stack_frame* frame) {
        const int max   = 24;
        int       level = 0;

        if (!frame)
            frame = (decltype(frame)) __builtin_frame_address(0);

        while (frame > (stack_frame*) 8) {
            if (skip != 0) {
                frame = frame->rbp;
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
            case ENTRY_SYSCALL:
                printk("  *syscall entry*\n");
                return;
            default:
                if (frame->rip == (size_t) Proc::Thread::exit_implicit) {
                    printk("  *thread entry/exit*\n");
                    return;
                }

                // printk("%p\n", cpu->local_tss->ist1);
                // printk("%p\n", cpu->local_tss->ist2);
                // printk("%p\n", cpu->local_tss->ist3);
                // printk("%p\n", cpu->local_tss->ist4);
                // printk("%p %p %i\n", cpu->reshed_stack_start, cpu, cpu->id);
                // printk("%p ? %p+%i %p+%i %p+%i\n", frame->rbp, fstk.ptr, fstk.size, kstk.ptr,
                //        kstk.size, cpu->reshed_stack_start, 4096);

                if (!is_valid(frame)) {
                    printk("  *assembly or g++ ignoring -fno-omit-frame-pointer*\n");
                    return;
                }

                int         delta = 0;
                const char* name  = addr2name((void*) frame->rip, delta);

                printk("  %p <%s+0x%x>\n", frame->rip, name ?: "no idea", delta);

                // if ((frame->rip & 0xFFFFFFFFF0000000) != 0xFFFFFFFF80000000)
                //     return;

                break;
            }

            frame = frame->rbp;

            // if (!Mem::kernel_pagemap->paddrof(frame))
            //     return;
        }
    }

    void stack_check() {
        if ((size_t) __builtin_frame_address(0) <= 0x00007FFFFFFFFFFF)
            kpanic("asdf\n");
    }

    void* caller(int depth) {
        if (!Proc::ready)
            return nullptr;

        stack_frame* frame = (decltype(frame)) __builtin_frame_address(0);
        const int    max   = 24;
        int          level = 0;

        while (frame > (stack_frame*) 8) {
            level++;
            if (level > max)
                return nullptr;

            switch (frame->rip) {
            case ENTRY_BOOT:
                return nullptr;
            case ENTRY_USER:
                return nullptr;
            case ENTRY_KERNEL:
                return nullptr;
            case ENTRY_SYSCALL:
                return nullptr;
            default:
                if (frame->rip == (size_t) Proc::Thread::exit_implicit)
                    return nullptr;

                break;
            }

            if (level == depth + 1)
                return (void*) frame->rip;

            if (!is_valid(frame))
                return nullptr;

            frame = frame->rbp;
            // if (!Mem::kernel_pagemap->paddrof(frame))
            //     return;
        }

        return nullptr;
    }
}