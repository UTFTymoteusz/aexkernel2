#pragma once

#include "aex/proc/types.hpp"

namespace AEX::Proc {
    class ProcessHandle;
    class ThreadHandle;

    class ProcessHandle {
        public:
        ProcessHandle(pid_t pid);

        ThreadHandle thread(tid_t tid);

        private:
        pid_t m_pid;
    };

    class ThreadHandle {
        public:
        ThreadHandle(pid_t pid, tid_t tid);

        ProcessHandle parent();

        private:
        pid_t m_pid;
        tid_t m_tid;
    };
}