#pragma once

#include "aex/rcparray.hpp"

#include "proc/process.hpp"
#include "proc/thread.hpp"

namespace AEX::Proc {
    extern RCPArray<Process> processes;
    extern Thread**          threads;

    void init();

    void setup_irq();

    void schedule();

    int add_process(Process* process);
    int add_thread(Thread* thread);
}