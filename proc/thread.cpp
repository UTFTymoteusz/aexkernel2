#include "proc/thread.hpp"

#include "aex/printk.hpp"

#include "proc/context.hpp"
#include "sys/cpu.hpp"
#include "sys/irq.hpp"

#include <stddef.h>

namespace AEX::Proc {
    extern "C" void proc_reshed();

    Thread::Thread(void* entry, void* stack, size_t stack_size, VMem::Pagemap* pagemap) {
        context = Context(entry, stack, stack_size, pagemap);
    }

    void Thread::yield() {
        proc_reshed();
    }

    void Thread::sleep(int ms) {
        auto currentThread = Sys::CPU::getCurrentCPU()->currentThread;

        currentThread->wakeup_at = (size_t) Sys::IRQ::get_curtime() + ms;
        currentThread->status    = Thread::state::SLEEPING;

        yield();
    }
}