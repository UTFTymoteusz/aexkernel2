#include "aex/proc/thread.hpp"

#include "aex/arch/ipc/signal.hpp"
#include "aex/arch/mem/helpers/stacker.hpp"
#include "aex/arch/sys/cpu.hpp"
#include "aex/ipc/signal.hpp"
#include "aex/ipc/types.hpp"
#include "aex/proc/setjmp.hpp"

// ASDAD
#include "aex/proc/process.hpp"

#include "signal.hpp"

namespace AEX::Proc {
    using namespace IPC;

    extern "C" void proc_ctxsave();
    extern "C" void proc_ctxload();

    static constexpr auto REDZONE_SIZE = 128;

    error_t Thread::sigpush(uint8_t id, sigaction& action, siginfo_t& info, state_combo scmb) {
        ASSERT(!Sys::CPU::checkInterrupts());

        size_t stack   = scmb.valid() ? scmb.stack() : context->rsp;
        auto   stacker = Mem::Stacker(stack);
        auto   frame   = scmb.valid() ? signal_registers(scmb) : signal_registers(context);

        if (Proc::setjmp(&Proc::Thread::current()->fault_recovery) == 0 &&
            stack <= Mem::kernel_pagemap->vstart) {
            stacker.move(REDZONE_SIZE);
            stacker.push(info);
            auto siginfo_rsp = stacker.pointer();

            stacker.push(m_sigqueue.mask);
            stacker.push(frame);
            auto frame_rsp = stacker.pointer();

            stacker.push(action.flags & SA_RESTORER ? action.restorer : sigret_trampoline);

            m_sigqueue.mask.block(action.mask);
            m_sigqueue.mask.add(info.si_signo);
            m_sigqueue.recalculate();

            if (scmb.valid()) {
                if (scmb.finfo_regs) {
                    scmb.finfo_regs->rip = (size_t) action.handler;
                    scmb.finfo_regs->rdi = id;
                    scmb.finfo_regs->rsi = siginfo_rsp;
                    scmb.finfo_regs->rdx = frame_rsp;
                    scmb.finfo_regs->rsp = stacker.pointer();
                }
                else {
                    scmb.syscall_regs->rip = (size_t) action.handler;
                    scmb.syscall_regs->rdi = id;
                    scmb.syscall_regs->rsi = siginfo_rsp;
                    scmb.syscall_regs->rdx = frame_rsp;
                    scmb.syscall_regs->rsp = stacker.pointer();
                }
            }
            else {
                ASSERT(context->usermode());

                context->rip = (size_t) action.handler;
                context->rdi = id;
                context->rsi = siginfo_rsp;
                context->rdx = frame_rsp;
                context->rsp = stacker.pointer();
            }

            Proc::nojmp(&Proc::Thread::current()->fault_recovery);
        }
        else {
            return EFAULT;
        }

        return ENONE;
    }

    error_t Thread::sigpop() {
        ASSERT(!Sys::CPU::checkInterrupts());

        auto stacker = Mem::Stacker(sigret_stack);

        if (Proc::setjmp(&Proc::Thread::current()->fault_recovery) == 0) {
            auto frame = stacker.pop<signal_registers>();
            ASSERT(frame);

            auto sigmask = stacker.pop<sigset_t>();
            ASSERT(sigmask);

            frame.value.inscribe(context);

            sigcheck_lock.acquireRaw();
            m_sigqueue.mask = sigmask;
            m_sigqueue.recalculate();
            sigcheck_lock.releaseRaw();
        }
        else {
            return EFAULT;
        }

        Proc::nojmp(&Proc::Thread::current()->fault_recovery);

        return ENONE;
    }
}