#pragma once

#include "aex/arch/proc/context.hpp"
#include "aex/arch/sys/cpu.hpp"
#include "aex/errno.hpp"
#include "aex/ipc/signal.hpp"
#include "aex/proc/types.hpp"

#include <stdint.h>

namespace AEX::IPC {
    struct API state_combo {
        Sys::CPU::fault_info* finfo_regs   = nullptr;
        syscall_registers*    syscall_regs = nullptr;

        state_combo() {}

        state_combo(Sys::CPU::fault_info* finfo) {
            finfo_regs = finfo;
        }

        state_combo(syscall_registers* sysregs) {
            syscall_regs = sysregs;
        }

        bool valid() {
            return finfo_regs || syscall_regs;
        }

        size_t stack();
    };

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
    } PACKED;

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

        signal_registers(state_combo& scmb) {
            if (scmb.finfo_regs) {
                r15    = scmb.finfo_regs->r15;
                r14    = scmb.finfo_regs->r14;
                r13    = scmb.finfo_regs->r13;
                r12    = scmb.finfo_regs->r12;
                r11    = scmb.finfo_regs->r11;
                r10    = scmb.finfo_regs->r10;
                r9     = scmb.finfo_regs->r9;
                r8     = scmb.finfo_regs->r8;
                rbp    = scmb.finfo_regs->rbp;
                rdi    = scmb.finfo_regs->rdi;
                rsi    = scmb.finfo_regs->rsi;
                rdx    = scmb.finfo_regs->rdx;
                rcx    = scmb.finfo_regs->rcx;
                rbx    = scmb.finfo_regs->rbx;
                rax    = scmb.finfo_regs->rax;
                rip    = scmb.finfo_regs->rip;
                rflags = scmb.finfo_regs->rflags;
                rsp    = scmb.finfo_regs->rsp;

                fxstate = scmb.finfo_regs->fxstate;
            }
            else {
                rsi    = scmb.syscall_regs->rsi;
                rdi    = scmb.syscall_regs->rdi;
                rdx    = scmb.syscall_regs->rdx;
                rax    = scmb.syscall_regs->rax;
                rflags = scmb.syscall_regs->rflags;
                rip    = scmb.syscall_regs->rip;
                rbp    = scmb.syscall_regs->rbp;
                rsp    = scmb.syscall_regs->rsp;

                fxstate = scmb.syscall_regs->fxstate;
            }
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

            ctx->fxstate = fxstate;
        }
    } PACKED;
}