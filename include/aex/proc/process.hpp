#pragma once

#include "aex/fs/descriptor.hpp"
#include "aex/ipc/event.hpp"
#include "aex/ipc/signal.hpp"
#include "aex/ipc/sigqueue.hpp"
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
        pid_t session = -1;
        pid_t group   = -1;

        char  name[64];
        char* image_path;

        int      nice;
        affinity cpu_affinity;
        rusage   usage;

        Spinlock lock;
        Spinlock threads_lock;
        Spinlock sigcheck_lock;
        Spinlock sigact_lock;
        Mutex    descs_mutex;

        int                             thread_counter;
        Mem::LazyVector<Thread_SP>      threads;
        Mem::LazyVector<FS::Descriptor> descs;

        Mem::Pagemap*                 pagemap;
        Mem::LazyVector<Mem::Region*> mmap_regions;

        Sys::syscall_t* syscall_table;

        IPC::Event        child_event;
        volatile status_t status;
        int               ret_code;
        bool              stopped;

        uint16_t tls_size;
        char*    tls_base;

        Sec::uid_t real_uid;
        Sec::uid_t eff_uid;
        Sec::uid_t saved_uid;

        Sec::gid_t real_gid;
        Sec::gid_t eff_gid;
        Sec::gid_t saved_gid;

        Process* next;
        Process* prev;

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
        static error_t kill(pid_t pid, IPC::siginfo_t info);

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

        // TODO: pls make this lock properly
        void        set_cwd(const char* cwd);
        const char* get_cwd();

        error_t kill(int sig);
        error_t kill(IPC::siginfo_t info);
        void    exit(int status);
        bool    exiting() {
            return m_exiting;
        }

        void                assoc(Thread* thread);
        optional<Thread_SP> unassoc(Thread* thread);
        optional<Thread_SP> get(tid_t tiddie);

        Mem::Vector<char*, 4>& env();
        void                   env(char* const envp[]);
        void                   env(Mem::Vector<char*, 4>* env);
        optional<char*>        envGet(int index);
        error_t                envSet(int index, char const* val);
        void                   clearEnv();

        bool isGroupLeader() {
            return group == pid;
        }

        bool isSessionLeader() {
            return session == pid;
        }

        // IPC Stuff
        error_t                  signal(IPC::siginfo_t& info);
        optional<IPC::sigaction> sigaction(uint8_t id);
        error_t                  sigaction(uint8_t id, IPC::sigaction& action);

        IPC::sigset_t getSignalPending();

        private:
        char*                 m_cwd;
        Mem::Vector<char*, 4> m_environment;

        bool           m_exiting = false;
        IPC::SigQueue  m_sigqueue;
        IPC::sigaction m_signals[72];

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