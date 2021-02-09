#include "ipc/ipc.hpp"

namespace AEX::IPC {
    void arch_signal_init();

    void init() {
        arch_signal_init();
    }
}