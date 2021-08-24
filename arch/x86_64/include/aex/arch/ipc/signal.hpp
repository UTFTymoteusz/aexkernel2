#pragma once

#include "aex/arch/proc/context.hpp"
#include "aex/errno.hpp"
#include "aex/ipc/signal.hpp"
#include "aex/proc/types.hpp"

#include <stdint.h>

namespace AEX::IPC {
    struct API signal_registers {
        uint64_t r15, r14, r13, r12, r11, r10, r9, r8, rbp, rdi, rsi, rdx, rcx, rbx, rax;
        uint64_t rip, rflags, rsp;

        Proc::fpu_context fxstate;

        signal_registers() {}
        signal_registers(Proc::Context* ctx) {
            r15    = ctx->r15;
            r14    = ctx->r14;
            r13    = ctx->r13;
            r12    = ctx->r12;
            r11    = ctx->r11;
            r10    = ctx->r10;
            r9     = ctx->r9;
            r8     = ctx->r8;
            rbp    = ctx->rbp;
            rdi    = ctx->rdi;
            rsi    = ctx->rsi;
            rdx    = ctx->rdx;
            rcx    = ctx->rcx;
            rbx    = ctx->rbx;
            rax    = ctx->rax;
            rip    = ctx->rip;
            rflags = ctx->rflags;
            rsp    = ctx->rsp;

            fxstate = ctx->fxstate;
        }

        void inscribe(Proc::Context* ctx) {
            ctx->r15    = r15;
            ctx->r14    = r14;
            ctx->r13    = r13;
            ctx->r12    = r12;
            ctx->r11    = r11;
            ctx->r10    = r10;
            ctx->r9     = r9;
            ctx->r8     = r8;
            ctx->rbp    = rbp;
            ctx->rdi    = rdi;
            ctx->rsi    = rsi;
            ctx->rdx    = rdx;
            ctx->rcx    = rcx;
            ctx->rbx    = rbx;
            ctx->rax    = rax;
            ctx->rip    = rip;
            ctx->rflags = rflags | 0x0202;
            ctx->rsp    = rsp;

            ctx->cs = 0x23;
            ctx->ss = 0x1B;

            ctx->fxstate = fxstate;
            ctx->fxstate.mxcsr &= 0x0000FFFF;
        }
    } PACKED;

    struct API syscall_registers {
        uint64_t rsi;
        uint64_t rdi;
        uint64_t rdx;
        uint64_t rax;
        uint64_t rflags;
        uint64_t rip;
        uint64_t rbp;
        uint64_t rsp;

        Proc::fpu_context fxstate;

        void inscribe(Proc::Context* ctx) {
            ctx->rsi    = rsi;
            ctx->rdi    = rdi;
            ctx->rdx    = rdx;
            ctx->rax    = rax;
            ctx->rflags = rflags;
            ctx->rip    = rip;
            ctx->rbp    = rbp;
            ctx->rsp    = rsp;

            ctx->fxstate = fxstate;
            ctx->fxstate.mxcsr &= 0x0000FFFF;
        }
    } PACKED;

    struct API interrupt_registers {
        Proc::fpu_context fxstate;
        uint64_t          r15, r14, r13, r12, r11, r10, r9, r8, rdi, rsi, rdx, rcx, rbx, rax, rbp;

        uint64_t int_no, err;
        uint64_t rip, cs, rflags, rsp, ss;

        void inscribe(Proc::Context* ctx) {
            ctx->r15    = r15;
            ctx->r14    = r14;
            ctx->r13    = r13;
            ctx->r12    = r12;
            ctx->r11    = r11;
            ctx->r10    = r10;
            ctx->r9     = r9;
            ctx->r8     = r8;
            ctx->rbp    = rbp;
            ctx->rdi    = rdi;
            ctx->rsi    = rsi;
            ctx->rdx    = rdx;
            ctx->rcx    = rcx;
            ctx->rbx    = rbx;
            ctx->rax    = rax;
            ctx->rip    = rip;
            ctx->rflags = rflags | 0x0202;
            ctx->rsp    = rsp;

            ctx->cs = 0x23;
            ctx->ss = 0x1B;

            ctx->fxstate = fxstate;
            ctx->fxstate.mxcsr &= 0x0000FFFF;
        }
    } PACKED;

    struct API state_combo {
        interrupt_registers* int_regs     = nullptr;
        syscall_registers*   syscall_regs = nullptr;

        state_combo() {}

        state_combo(interrupt_registers* ctx) {
            int_regs = ctx;
        }

        state_combo(syscall_registers* ctx) {
            syscall_regs = ctx;
        }

        void write(signal_registers* regs) {
            if (int_regs) {
                regs->r15 = int_regs->r15;
                regs->r14 = int_regs->r14;
                regs->r13 = int_regs->r13;
                regs->r12 = int_regs->r12;
                regs->r11 = int_regs->r11;
                regs->r10 = int_regs->r10;
                regs->r9  = int_regs->r9;
                regs->r8  = int_regs->r8;
                regs->rdi = int_regs->rdi;
                regs->rsi = int_regs->rsi;
                regs->rdx = int_regs->rdx;
                regs->rcx = int_regs->rcx;
                regs->rbx = int_regs->rbx;
                regs->rax = int_regs->rax;
                regs->rbp = int_regs->rbp;

                regs->rip    = int_regs->rip;
                regs->rsp    = int_regs->rsp;
                regs->rflags = int_regs->rflags;

                memcpy(&regs->fxstate, &int_regs->fxstate, sizeof(int_regs->fxstate));
            }

            if (syscall_regs) {
                regs->rsi    = syscall_regs->rsi;
                regs->rdi    = syscall_regs->rdi;
                regs->rdx    = syscall_regs->rdx;
                regs->rax    = syscall_regs->rax;
                regs->rflags = syscall_regs->rflags;
                regs->rip    = syscall_regs->rip;
                regs->rbp    = syscall_regs->rbp;
                regs->rsp    = syscall_regs->rsp;

                memcpy(&regs->fxstate, &syscall_regs->fxstate, sizeof(syscall_regs->fxstate));
            }
        }
    };

    error_t signal_push_context(Proc::Thread* thread, uint8_t id, sigaction& action,
                                state_combo* combo, siginfo_t& info);
    error_t signal_pop_context(Proc::Thread* thread);
}