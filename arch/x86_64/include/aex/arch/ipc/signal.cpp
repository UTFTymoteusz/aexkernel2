#include "signal.hpp"

namespace AEX::IPC {
    size_t state_combo::stack() {
        return finfo_regs ? finfo_regs->rsp : syscall_regs->rsp;
    }
}