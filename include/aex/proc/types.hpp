#pragma once

#include "aex/types.hpp"

namespace AEX::Proc {
    enum status_t : uint8_t {
        TS_FRESH = 0,
        TS_RUNNABLE,
        TS_SLEEPING,
        TS_BLOCKED,
        TS_DEAD,
    };

    typedef int pid_t;
    typedef int tid_t;

    class Process;
    class Thread;

    API const char* strstatus(status_t status);
}
