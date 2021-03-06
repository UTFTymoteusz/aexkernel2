#pragma once

#include "aex/mem.hpp"
#include "aex/proc.hpp"

namespace AEX::Proc {
    extern Mutex processes_lock;

    extern int      process_list_size;
    extern Process* process_list_head;
    extern Process* process_list_tail;

    extern Spinlock sched_lock;

    extern int     thread_list_size;
    extern Thread* thread_list_head;
    extern Thread* thread_list_tail;

    extern Thread** idle_threads;

    extern bool ready;

    void init();

    /**
     * Setups the interrupts for preemptive multitasking.
     **/
    void setup_irq();

    /**
     * Picks the next thread to run on the local CPU.
     **/
    void schedule();

    void debug_print_cpu_jobs();
    void debug_print_threads();
    void debug_print_idles();
    void debug_print_processes();
}