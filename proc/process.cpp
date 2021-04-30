#include "aex/proc/process.hpp"

#include "aex/assert.hpp"
#include "aex/fs.hpp"
#include "aex/mem.hpp"
#include "aex/proc.hpp"
#include "aex/proc/broker.hpp"
#include "aex/string.hpp"
#include "aex/sys/syscall.hpp"

#include "proc/proc.hpp"

// #define STABILITY_TEST
// #define STABLE_MARK 500000

namespace AEX::Proc {
    Process::Process(const char* image_path, pid_t parent_pid, Mem::Pagemap* pagemap,
                     const char* name) {
#ifdef STABILITY_TEST
        static int  checkpoint   = 100;
        static int  off          = 0;
        static int  coff         = 0;
        static char ccs[16]      = {'|', '/', '-', '\\', '|', '/', '-', '\\'};
        static int  cc           = '|';
        static char progress[16] = {':', ':', ':', ':', ':', ':', ':', ':', ':', ':'};
#endif

        rename(image_path, name);
        ipc_init();

        auto scope = processes_lock.scope();

        this->parent_pid        = parent_pid;
        this->status            = TS_FRESH;
        this->usage.cpu_time_ns = 0;
        this->pid               = add_process(this);
        this->pagemap           = pagemap;
        this->syscall_table     = Sys::default_table();

#ifdef STABILITY_TEST
        if (this->pid > checkpoint) {
            if (checkpoint % 1000 == 0) {
                printk("         pid checkpoint %i/%i passed", checkpoint, STABLE_MARK);

                if (checkpoint >= STABLE_MARK) {
                    printk(" (threading actually works now?)\n");
                    kcalmness("Threading is fixed, I think");
                }
            }

            checkpoint += 100;

            progress[off] = ':';
            off++;

            if (off == 10) {
                off = 0;

                coff++;
                if (coff >= 8)
                    coff = 0;

                cc = ccs[coff];
            }

            progress[off] = cc;

            printk("\r%s\r", progress);
        }
#endif
    }

    Process::~Process() {
        disposed = true;

        if (pagemap != Mem::kernel_pagemap)
            delete pagemap;

        delete m_cwd;
        delete image_path;
        clearEnv();

        remove_process(this);
    }

    void Process::ready() {
        status = TS_RUNNABLE;
    }

    void Process::rename(const char* image_path_n, const char* name_n) {
        if (name_n == nullptr)
            FS::get_filename(name, image_path_n, sizeof(name));
        else
            strncpy(name, name_n, sizeof(name));

        image_path = Mem::Heap::realloc(image_path, strlen(image_path_n) + 1);
        strncpy(image_path, image_path_n, strlen(image_path_n) + 1);
    }

    void Process::set_cwd(const char* cwd) {
        auto scope = lock.scope();

        int len = min(strlen(cwd), FS::MAX_PATH_LEN - 1);

        m_cwd = Mem::Heap::realloc(m_cwd, len + 1);
        strncpy(m_cwd, cwd, len + 1);
    }

    const char* Process::get_cwd() {
        auto scope = lock.scope();
        return m_cwd;
    }

    Process* Process::current() {
        return Thread::current()->parent;
    }

    Process* Process::kernel() {
        return (Process*) process_list_head->next;
    }

    void exit_threaded(Process* process) {
        process->threads_lock.acquire();
        for (int i = 0; i < process->threads.count(); i++) {
            if (!process->threads.present(i))
                continue;

            process->threads.at(i)->abort(true);
        }
        process->threads_lock.release();

        while (Mem::atomic_read(&process->thread_counter) != 0)
            Proc::Thread::sleep(50);

        process->lock.acquire();

        for (int i = 0; i < process->mmap_regions.count(); i++) {
            if (!process->mmap_regions.present(i))
                continue;

            auto region = process->mmap_regions.at(i);
            process->mmap_regions.erase(i);

            delete region;
        }

        process->lock.release();

        process->descs_lock.acquire();

        for (int i = 0; i < process->descs.count(); i++) {
            if (!process->descs.present(i))
                continue;

            process->descs.at(i).file->close();
        }

        process->descs_lock.release();

        processes_lock.acquire();
        process->status = TS_DEAD;

        auto iter_process = process_list_head;
        for (int i = 0; i < process_list_size; i++) {
            if (iter_process->parent_pid == process->pid)
                iter_process->parent_pid = process->parent_pid;

            iter_process = iter_process->next;
        }

        auto parent = get_process(process->parent_pid);

        parent->lock.acquire();
        parent->child_event.raise();
        parent->lock.release();

        // PRINTK_DEBUG1("pid%i: full exit", process->pid);

        processes_lock.release();
    }

    void exit_threaded_broker(Process* process) {
        auto thread_try = threaded_call(exit_threaded, process);
        AEX_ASSERT(thread_try);

        thread_try->detach();
    }

    void Process::exit(int status) {
        lock.acquire();

        if (m_exiting) {
            PRINTK_DEBUG_WARN("exit() while already exitting");
            lock.release();
            return;
        }

        m_exiting = true;
        ret_code  = status;

        PRINTK_DEBUG2("pid%i: exit(%i)", pid, status);

        broker(exit_threaded_broker, this);

        lock.release();

        if (Process::current() == this)
            while (!Thread::current()->aborting())
                Thread::yield();
    }

    error_t Process::kill(pid_t pid, int sig) {
        auto scope   = processes_lock.scope();
        auto process = get_process(pid);
        if (!process)
            return ESRCH;

        if (process->status == TS_FRESH)
            return ESRCH;

        IPC::siginfo_t info;

        info.si_signo = sig;
        info.si_uid   = process->real_uid;
        info.si_code  = SI_USER;

        return process->signal(info);
    }

    struct wait_args {
        pid_t pid;
        int   code;

        wait_args() {}

        wait_args(pid_t pid, int code) {
            this->pid  = pid;
            this->code = code;
        }
    };

    optional<wait_args> try_get(int pid) {
        AEX_ASSERT(!processes_lock.tryAcquire());

        auto    process = process_list_head;
        error_t error   = ECHILD;

        for (int i = 0; i < process_list_size; i++) {
            if (process->parent_pid != pid) {
                process = process->next;
                continue;
            }

            error = EAGAIN;

            if (process->status != TS_DEAD) {
                process = process->next;
                continue;
            }

            pid_t pid   = process->pid;
            int   rcode = process->ret_code;

            delete process;
            return wait_args(pid, rcode);
        }

        return error;
    }

    optional<pid_t> Process::wait(int& status) {
        while (true) {
            processes_lock.acquire();
            auto val = try_get(Process::current()->pid);

            if (!val && val.error_code == ECHILD) {
                processes_lock.release();
                return val.error_code;
            }

            if (val) {
                // PRINTK_DEBUG2("pid%i: cleaned up pid%i", Process::current()->pid, val.value.pid);

                status = val.value.code;
                processes_lock.release();
                return val.value.pid;
            }

            Process::current()->lock.acquire();
            Process::current()->child_event.wait();

            // The processes lock needs to be released before the process lock in order to avoid a
            // deadlock (TS_BLOCKED and mutex holding)
            processes_lock.release();
            Process::current()->lock.release();

            Thread::yield();

            if (Thread::current()->interrupted())
                return EINTR;
        }
    }

    void Process::assoc(Thread* thread) {
        threads_lock.acquire();
        threads.push(thread);
        threads_lock.release();
    }

    void Process::unassoc(Thread* thread) {
        threads_lock.acquire();

        for (int i = 0; i < threads.count(); i++) {
            if (!threads.present(i) || threads[i] != thread)
                continue;

            threads.erase(i);
            break;
        }

        threads_lock.release();
    }

    Mem::Vector<char*, 4>& Process::env() {
        AEX_ASSERT(lock.isAcquired());
        return m_environment;
    }

    void Process::env(char* const envp[]) {
        clearEnv();

        auto scope = lock.scope();

        for (int i = 0; i < 256; i++) {
            if (!envp[i])
                break;

            char const* var   = envp[i];
            char*       var_d = new char[strlen(var) + 1];

            m_environment.push(var_d);
            strncpy(var_d, var, (size_t) strlen(var) + 1);
        }
    }

    void Process::env(Mem::Vector<char*, 4>* env) {
        clearEnv();

        auto scope = lock.scope();
        for (int i = 0; i < env->count(); i++) {
            char const* var   = env->at(i);
            char*       var_d = new char[strlen(var) + 1];

            m_environment.push(var_d);
            strncpy(var_d, var, (size_t) strlen(var) + 1);
        }
    }

    void Process::clearEnv() {
        auto scope = lock.scope();
        for (int i = 0; i < m_environment.count(); i++)
            delete m_environment[i];

        m_environment.clear();
    }

    optional<char*> Process::envGet(int index) {
        AEX_ASSERT(lock.isAcquired());

        if (index < 0 || index >= m_environment.count())
            return EINVAL;

        return m_environment[index];
    }

    error_t Process::envSet(int index, char const* val) {
        AEX_ASSERT(lock.isAcquired());

        if (index == -1) {
            int   len    = strlen(val);
            char* buffer = new char[len + 1];

            m_environment.push(buffer);
            strncpy(buffer, val, len + 1);

            return ENONE;
        }

        if (index < 0 || index >= m_environment.count())
            return EINVAL;

        int len = strlen(val);

        m_environment[index] = (char*) Mem::Heap::realloc(m_environment[index], len + 1);
        strncpy(m_environment[index], val, len + 1);

        return ENONE;
    }
}