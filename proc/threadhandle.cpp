#include "aex/proc/handles.hpp"

namespace AEX::Proc {
    ThreadHandle::ThreadHandle(pid_t pid, tid_t tid) {
        m_pid = pid;
        m_tid = tid;
    }

    ProcessHandle ThreadHandle::parent() {
        return ProcessHandle(m_pid);
    }
}
