#pragma once

#include "aex/proc/process.hpp"
#include "aex/proc/thread.hpp"
#include "aex/rcparray.hpp"

namespace AEX::Proc {
    extern RCPArray<Process> processes;
    extern Thread**          threads;

    void init();

    void setup_irq();

    void schedule();

    int add_process(Process* process);
    int add_thread(Thread* thread);
}