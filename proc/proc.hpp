#pragma once

#include "aex/mem.hpp"
#include "aex/proc.hpp"

namespace AEX::Proc {
    extern Mem::SmartArray<Process> processes;

    extern Spinlock lock;

    extern Thread** idle_threads;

    extern int     thread_list_size;
    extern Thread* thread_list_head;
    extern Thread* thread_list_tail;

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

    void add_thread(Thread* thread);
    void remove_thread(Thread* thread);

    void debug_print_cpu_jobs();
    void debug_print_list();
    void debug_print_processes();
}