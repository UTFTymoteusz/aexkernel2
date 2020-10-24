#pragma once

#include "aex/ipc/event.hpp"
#include "aex/mem.hpp"
#include "aex/mem/lazyvector.hpp"
#include "aex/mem/mmap.hpp"
#include "aex/proc.hpp"
#include "aex/proc/affinity.hpp"
#include "aex/proc/resource_usage.hpp"
#include "aex/proc/types.hpp"
#include "aex/sys/syscall.hpp"

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
        Mutex    thread_lock;

        int                          thread_counter;
        Mem::LazyVector<Thread*>     threads;
        Mem::LazyVector<FS::File_SP> files;

        Mem::Pagemap*                     pagemap;
        Mem::LazyVector<Mem::MMapRegion*> mmap_regions;

        Sys::syscall_t* syscall_table;

        Process* next;
        Process* prev;

        IPC::Event child_event;
        int        status;
        int        ret_code;

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

        static error_t         kill(pid_t pid);
        static optional<pid_t> wait(int& status);

        /**
         * Gets the current process.
         * @returns The pointer to the process.
         */
        static Process* current();

        void ready();

        void exit(int status);

        private:
        bool m_exiting = false;
    };
}