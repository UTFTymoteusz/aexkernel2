#pragma once

#include "aex/mem/smartarray.hpp"
#include "aex/mem/vector.hpp"
#include "aex/proc/process.hpp"
#include "aex/proc/thread.hpp"

namespace AEX::Proc {
    extern Mem::SmartArray<Process> processes;
    extern Thread**                 threads;

    void init();

    /**
     * Setups the interrupts for preemptive multitasking.
     */
    void setup_irq();

    /**
     * Picks the next thread to run on the local CPU.
     */
    void schedule();

    pid_t add_process(Process* process);
    tid_t add_thread(Thread* thread);

    void abort_thread(Thread* thread);

    void reap_thread(Thread* thread);

    void debug_print_cpu_jobs();
}