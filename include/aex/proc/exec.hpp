#pragma once

#include "aex/errno.hpp"
#include "aex/fs/file.hpp"
#include "aex/mem/vector.hpp"
#include "aex/mutex.hpp"
#include "aex/proc/process.hpp"
#include "aex/utility.hpp"

namespace AEX::Proc {
    static constexpr auto ARG_MAX  = 8192; // Maximum total length of arguments
    static constexpr auto ARGC_MAX = 64;   // Maximum count of arguments

    static constexpr auto ENV_MAX  = 32768; // Maximum total length of the environment
    static constexpr auto ENVL_MAX = 8192;  // Maximum total length for an environment line
    static constexpr auto ENVC_MAX = 128;   // Maximum count of environment variables

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
