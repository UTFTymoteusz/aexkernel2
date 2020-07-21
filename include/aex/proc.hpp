#pragma once

#include "aex/debug.hpp"
#include "aex/mem/smartarray.hpp"
#include "aex/mem/vector.hpp"
#include "aex/printk.hpp"
#include "aex/proc/process.hpp"
#include "aex/proc/thread.hpp"

namespace AEX::Proc {
    extern Mem::SmartArray<Process> processes;
    extern Thread**                 threads;

    int add_process(Process* process);
    int add_thread(Thread* thread);

    /**
     * Creates a new thread that's gonna run the specified function with the specified arguments.
     * @param func Function to call.
     * @param args Function arguments.
     * @returns The created thread.
     */
    template <typename Func, typename... Args>
    Thread* threaded_call(Func func, Args... args) {
        auto thread = new Thread(nullptr, (void*) func, Thread::KERNEL_STACK_SIZE, nullptr);

        thread->setArguments(args...);
        thread->start();

        return thread;
    }
}