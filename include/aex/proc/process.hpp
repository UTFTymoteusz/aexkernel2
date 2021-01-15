#pragma once

#include "aex/fs/file.hpp"
#include "aex/ipc/event.hpp"
#include "aex/ipc/signal.hpp"
#include "aex/mem.hpp"
#include "aex/mem/mmap.hpp"
#include "aex/mutex.hpp"
#include "aex/optional.hpp"
#include "aex/proc.hpp"
#include "aex/proc/affinity.hpp"
#include "aex/proc/resource_usage.hpp"
#include "aex/proc/types.hpp"
#include "aex/sec/types.hpp"
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

        int                      thread_counter;
        Mutex                    threads_lock;
        Mem::LazyVector<Thread*> threads;

        Spinlock                     files_lock;
        Mem::LazyVector<FS::File_SP> files;

        Mem::Pagemap*                     pagemap;
        Mem::LazyVector<Mem::MMapRegion*> mmap_regions;

        Sys::syscall_t* syscall_table;

        Process* next;
        Process* prev;

        IPC::Event child_event;
        int        status;
        int        ret_code;

        uint16_t tls_size;

        Sec::uid_t real_uid;
        Sec::uid_t eff_uid;
        Sec::uid_t saved_uid;

        Sec::gid_t real_gid;
        Sec::gid_t eff_gid;
        Sec::gid_t saved_gid;

        Process() = default;

        /**
         * Creates a process and puts it in the processes RCPArray.
         * @param image_path Image path.
         * @param name       Process name. Will get generated from the image path if not specified.
         * @param parent_pid PID of the parent process.
         **/
        Process(const char* image_path, pid_t parent_pid, Mem::Pagemap* pagemap,
                const char* name = nullptr);

        ~Process();

        static error_t kill(pid_t pid, int sig);

        /**
         * Waits for any child process to exit.
         * @param status Reference to an int where the exit code will be returned.
         * @returns PID of the process which exited or an error code.
         **/
        static optional<pid_t> wait(int& status);

        /**
         * Gets the kernel process.
         * @returns The pointer to the kernel process.
         **/
        static Process* kernel();

        /**
         * Gets the current process.
         * @returns The pointer to the process.
         **/
        static Process* current();

        void ready();
        void rename(const char* image_path, const char* name);

        void        set_cwd(const char* cwd);
        const char* get_cwd();

        void exit(int status);

        void assoc(Thread* thread);
        void unassoc(Thread* thread);

        // IPC Stuff
        /**
         *
         **/
        error_t                  signal(IPC::siginfo_t& info);
        optional<IPC::sigaction> sigaction(uint8_t id);
        error_t                  sigaction(uint8_t id, IPC::sigaction& action);

        private:
        bool  m_exiting = false;
        char* m_cwd;

        IPC::sigaction m_signals[32];

        void ipc_init();

        friend class Thread;
    };
}