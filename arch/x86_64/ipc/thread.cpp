#include "aex/proc/thread.hpp"

#include "aex/arch/ipc/signal.hpp"
#include "aex/arch/mem/helpers/usr_stack.hpp"
#include "aex/ipc/signal.hpp"
#include "aex/ipc/types.hpp"

#include "signal.hpp"

namespace AEX::Proc {
    using namespace IPC;

    extern "C" void proc_ctxsave();
    extern "C" void proc_ctxload();

    static constexpr auto REDZONE_SIZE = 128;

    error_t Thread::pushSignalContext(uint8_t id, sigaction& action, state_combo* combo,
                                      siginfo_t& info) {
        AEX_ASSERT(!Sys::CPU::checkInterrupts());

        if (combo)
            proc_ctxsave();

        auto stack = Mem::usr_stack(this, saved_stack);
        auto frame = signal_registers(context);

        if (combo)
            combo->write(&frame);

        stack.move(REDZONE_SIZE);
        stack.push(info);
        auto siginfo_rsp = stack.pointer();

        stack.push(m_sigqueue.mask);
        stack.push(frame);
        auto frame_rsp = stack.pointer();

        stack.push(action.flags & SA_RESTORER ? action.restorer : sigret_trampoline);

        m_sigqueue.mask.block(action.mask);
        m_sigqueue.mask.add(info.si_signo);
        m_sigqueue.recalculate();

        context->rip = (size_t) action.handler;
        context->rdi = id;
        context->rsi = siginfo_rsp;
        context->rdx = frame_rsp;

        context->rsp = stack.pointer();
        context->cs  = 0x23;
        context->ss  = 0x1B;

        if (combo)
            sigcheck_lock.releaseRaw();

        return ENONE;
    }

    error_t Thread::popSignalContext() {
        AEX_ASSERT(!Sys::CPU::checkInterrupts());

        auto stack = Mem::usr_stack(this, saved_stack);
        auto frame = stack.pop<signal_registers>();
        AEX_ASSERT(frame);

        auto sigmask = stack.pop<sigset_t>();
        AEX_ASSERT(sigmask);

        frame.value.inscribe(context);

        sigcheck_lock.acquireRaw();
        m_sigqueue.mask = sigmask;
        m_sigqueue.recalculate();
        sigcheck_lock.releaseRaw();

        return ENONE;
    }
}