#pragma once

#include "aex/debug.hpp"
#include "aex/mem/smartarray.hpp"
#include "aex/mem/vector.hpp"
#include "aex/printk.hpp"
#include "aex/proc/handles.hpp"
#include "aex/proc/process.hpp"
#include "aex/proc/thread.hpp"
#include "aex/proc/types.hpp"

namespace AEX::Proc {
    /**
     * Creates a new thread that's gonna run the specified function with the specified arguments.
     * @param func Function to call.
     * @param args Function arguments.
     * @returns The created thread.
     **/
    template <typename Func, typename... Args>
    [[nodiscard]] Thread* threaded_call(Func func, Args... args) {
        auto thread_try = Thread::create(1, (void*) func, Thread::KERNEL_STACK_SIZE, nullptr);
        if (!thread_try)
            return nullptr;

        auto thread = thread_try.value;

        thread->setArguments(args...);
        thread->start();

        return thread;
    }
}