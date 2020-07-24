#pragma once

#include "aex/mem.hpp"
#include "aex/proc.hpp"

namespace AEX::Proc {
    extern Mem::SmartArray<Process> processes;
    extern Thread**                 threads;

    extern bool ready;

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