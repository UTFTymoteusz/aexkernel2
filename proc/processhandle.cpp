#include "aex/proc/handles.hpp"

#include "proc/proc.hpp"

namespace AEX::Proc {
    ProcessHandle::ProcessHandle(pid_t pid) {
        m_pid = pid;
    }

    ThreadHandle ProcessHandle::thread(tid_t tid) {
        return ThreadHandle(m_pid, tid);
    }
}