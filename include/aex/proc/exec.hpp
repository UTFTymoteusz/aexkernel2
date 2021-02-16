#pragma once

#include "aex/errno.hpp"
#include "aex/fs/file.hpp"
#include "aex/mem/vector.hpp"
#include "aex/mutex.hpp"
#include "aex/proc/process.hpp"
#include "aex/utility.hpp"

namespace AEX::Proc {
    class API Executor {
        public:
        virtual error_t exec(Process* process, Thread* initiator, const char* path,
                             char* const argv[], char* const envp[]);
    };

    struct API exec_opt {
        FS::File_SP stdin, stdout, stderr;
    };

    API void    register_executor(Executor* executor);
    API error_t exec(Process* process, Thread* initiator, const char* path, char* const argv[],
                     char* const envp[], exec_opt* options);
}
