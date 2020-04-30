#pragma once

#include "proc/thread.hpp"

namespace AEX::Proc {
    typedef int pid;

    class Process {
      public:
        char name[64];
        pid  parent_pid;

        Process() = default;
        Process(const char* name, pid parent_pid);

      private:
    };
}