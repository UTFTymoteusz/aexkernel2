#pragma once

#include "aex/errno.hpp"
#include "aex/mem/vector.hpp"
#include "aex/mutex.hpp"
#include "aex/proc/process.hpp"

namespace AEX::Proc {
    class Executor {
        public:
        virtual error_t exec(const char* path, Process* process);
    };

    void    registerExecutor(Executor* executor);
    error_t exec(const char* path);
}
