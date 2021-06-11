#pragma once

#include "aex/fs/descriptor.hpp"
#include "aex/ipc/event.hpp"
#include "aex/ipc/signal.hpp"
#include "aex/mem.hpp"
#include "aex/mem/mmap.hpp"
#include "aex/mutex.hpp"
#include "aex/optional.hpp"
#include "aex/proc.hpp"
#include "aex/proc/affinity.hpp"
#include "aex/proc/rusage.hpp"
#include "aex/proc/types.hpp"
#include "aex/sec/types.hpp"
#include "aex/sys/syscall.hpp"
#include "aex/utility.hpp"

namespace AEX::Mem {
    class Pagemap;
}

namespace AEX::Proc {
    class API Process {
        public:
        pid_t pid;
        pid_t parent_pid;

        char  name[64];
        char* image_path;

        affinity cpu_affinity;
        rusage   usage;

        Spinlock lock;
        Spinlock threads_lock;
        Mutex    descs_mutex;

        int                             thread_counter;
        Mem::LazyVector<Thread*>        threads;
        Mem::LazyVector<FS::Descriptor> descs;

        Mem::Pagemap*                 pagemap;
        Mem::LazyVector<Mem::Region*> mmap_regions;

        Sys::syscall_t* syscall_table;

        Process* next;
        Process* prev;

        IPC::Event        child_event;
        volatile status_t status;
        int               ret_code;

        uint16_t tls_size;

        int nice;

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

        Mem::Vector<char*, 4>& env();
        void                   env(char* const envp[]);
        void                   env(Mem::Vector<char*, 4>* env);
        void                   clearEnv();

        optional<char*> envGet(int index);
        error_t         envSet(int index, char const* val);

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

        IPC::sigaction        m_signals[32];
        Mem::Vector<char*, 4> m_environment;

        void ipc_init();

        void purge_threads();
        void purge_mmaps();
        void purge_descriptors();

        friend void exit_threaded(Process* process);
        friend class Thread;
    };

    API pid_t add_process(Process* process);
    API void  remove_process(Process* process);
    API Process* get_process(pid_t pid);
}