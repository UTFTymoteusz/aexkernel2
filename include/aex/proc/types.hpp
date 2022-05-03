#pragma once

#include "aex/types.hpp"

namespace AEX::Proc {
    enum status_t : uint8_t {
        TS_FRESH = 0,
        TS_RUNNABLE,
        TS_SLEEPING,
        TS_BLOCKED,
        TS_HELD,
        TS_VERYSPECIAL,
        TS_DEAD,
    };

    struct stack {
        void*  ptr;
        size_t size;
    };

    typedef int pid_t;
    typedef int tid_t;

    class Process;
    class Thread;

    typedef Mem::SmartPointer<Process> Process_SP;
    typedef Mem::SmartPointer<Thread>  Thread_SP;

    API const char* strstatus(status_t status);
}
