#pragma once

#include "proc/resource_usage.hpp"
#include "proc/thread.hpp"

namespace AEX::Proc {
    typedef int pid_t;

    class Process {
      public:
        pid_t pid;
        pid_t parent_pid;

        char  name[64];
        char* image_path;

        resource_usage usage;

        Process() = default;

        /**
         * Creates a process and puts it in the processes RCPArray.
         * @param image_path Image path.
         * @param name       Process name. Will get generated from the image path if not specified;
         * @param parent_pid PID of the parent process.
         */
        Process(const char* image_path, pid_t parent_pid, const char* name = nullptr);

      private:
    };
}