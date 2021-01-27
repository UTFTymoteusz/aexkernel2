#pragma once

#include "aex/errno.hpp"
#include "aex/ipc/signal.hpp"
#include "aex/proc/types.hpp"

#include <stdint.h>

namespace AEX::IPC {
    error_t signal_context(Proc::Thread* thread, uint8_t id, sigaction& action);
    error_t signal_context(Proc::Thread* thread, uint8_t id, sigaction& action, siginfo_t& info);
}