#pragma once

#include "aex/errno.hpp"
#include "aex/fs/file.hpp"
#include "aex/mem/vector.hpp"
#include "aex/mutex.hpp"
#include "aex/proc/process.hpp"

namespace AEX::Proc {
    class Executor {
        public:
        virtual error_t exec(Process* process, Thread* initiator, const char* path,
                             char* const argv[], char* const envp[]);
    };

    struct exec_opt {
        FS::File_SP stdin, stdout, stderr;
    };

    void    register_executor(Executor* executor);
    error_t exec(Process* process, Thread* initiator, const char* path, char* const argv[],
                 char* const envp[], exec_opt* options);
}
