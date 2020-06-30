#pragma once

#include "aex/mem/smartarray.hpp"
#include "aex/mem/vector.hpp"
#include "aex/proc/process.hpp"
#include "aex/proc/thread.hpp"

namespace AEX::Proc {
    extern Mem::SmartArray<Process> processes;
    extern Thread**                 threads;

    int add_process(Process* process);
    int add_thread(Thread* thread);
}