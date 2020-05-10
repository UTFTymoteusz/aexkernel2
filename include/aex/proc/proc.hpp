#pragma once

#include "aex/mem/smartarray.hpp"
#include "aex/mem/vector.hpp"

namespace AEX::Proc {
    class Process;
    class Thread;

    extern Mem::SmartArray<Process> processes;
    extern Mem::Vector<Thread*>     threads;

    int add_process(Process* process);
    int add_thread(Thread* thread);
}