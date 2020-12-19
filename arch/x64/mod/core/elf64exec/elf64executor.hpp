#pragma once

#include "aex/proc/exec.hpp"

class Elf64Executor : public AEX::Proc::Executor {
    public:
    AEX::error_t exec(AEX::Proc::Process* process, AEX::Proc::Thread* initiator, const char* path,
                      char* const argv[], char* const envp[]);

    private:
    void abortall(AEX::Proc::Process* process, AEX::Proc::Thread* except = nullptr);
    void setup_argv(AEX::Proc::Process* process, char* const argv[], int& argc_out,
                    char**& argv_out);
};