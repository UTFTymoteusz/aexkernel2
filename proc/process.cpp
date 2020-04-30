#include "proc/process.hpp"

#include "aex/string.hpp"

namespace AEX::Proc {
    Process::Process(const char* name, pid parent_pid) {
        strncpy(this->name, name, sizeof(this->name));

        this->parent_pid = parent_pid;
    }
}