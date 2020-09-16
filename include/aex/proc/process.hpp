#pragma once

#include "aex/mem.hpp"
#include "aex/mem/lazyvector.hpp"
#include "aex/mem/mmap.hpp"
#include "aex/proc.hpp"
#include "aex/proc/affinity.hpp"
#include "aex/proc/resource_usage.hpp"
#include "aex/proc/types.hpp"

namespace AEX::Mem {
    class Pagemap;
}

namespace AEX::Proc {
    class Process {
        public:
        pid_t pid;
        pid_t parent_pid;

        char  name[64];
        char* image_path;

        affinity       cpu_affinity;
        resource_usage usage;

        Spinlock lock;

        Mem::LazyVector<Thread*, nullptr> threads;

        Mem::Pagemap*                 pagemap;
        Mem::Vector<Mem::MMapRegion*> mmap_regions;

        Process() = default;

        /**
         * Creates a process and puts it in the processes RCPArray.
         * @param image_path Image path.
         * @param name       Process name. Will get generated from the image path if not specified.
         * @param parent_pid PID of the parent process.
         */
        Process(const char* image_path, pid_t parent_pid, Mem::Pagemap* pagemap,
                const char* name = nullptr);

        ~Process();

        /**
         * Gets the current process.
         * @returns The SmartPointer to the process.
         */
        static Mem::SmartPointer<Process> current();

        private:
    };

    typedef Mem::SmartPointer<Process> Process_SP;
}