#include "aex/proc/process.hpp"

#include "aex/assert.hpp"
#include "aex/bool.hpp"
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
        SCOPE(lock);

        if (name_n == nullptr)
            FS::get_filename(name, image_path_n, sizeof(name));
        else
            strlcpy(name, name_n, sizeof(name));

        auto old_image_path = image_path;
        image_path          = strndup(image_path_n, FS::PATH_MAX - 1);

        if (old_image_path)
            delete old_image_path;
    }

    void Process::set_cwd(const char* cwd) {
        SCOPE(lock);

        auto old_cwd = m_cwd;
        m_cwd        = strndup(cwd, FS::PATH_MAX - 1);

        if (old_cwd)
            delete old_cwd;
    }

    const char* Process::get_cwd() {
        SCOPE(lock);
        return m_cwd;
    }

    Process* Process::current() {
        return Thread::current()->parent;
    }

    Process* Process::kernel() {
        return (Process*) process_list_head->next;
    }

    void exit_threaded(Process* process) {
        process->purge_threads();
        process->purge_mmaps();
        process->purge_descriptors();

        SCOPE(processes_lock);

        process->status = TS_DEAD;

        auto iter_process = process_list_head;
        for (int i = 0; i < process_list_size; i++) {
            if (iter_process->parent_pid == process->pid)
                iter_process->parent_pid = process->parent_pid;

            iter_process = iter_process->next;
        }

        auto parent = get_process(process->parent_pid);
        using(parent->lock) {
            parent->child_event.raise();
        }

        printkd(PTKD_PROC, "proc: pid%i: Full exit\n", process->pid);
    }

    void exit_threaded_broker(Process* process) {
        auto thread_try = threaded_call(exit_threaded, process);
        AEX_ASSERT(thread_try);

        thread_try->detach();
    }

    void Process::exit(int status) {
        lock.acquire();

        if (m_exiting) {
            printkd(PTKD_PROC, WARN "proc: pid%i: exit(%i) while already exitting\n", pid, status);
            lock.release();
            return;
        }

        m_exiting = true;
        ret_code  = status;

        printkd(PTKD_PROC, "proc: pid%i: exit(%i)\n", pid, status);
        broker(exit_threaded_broker, this);

        lock.release();

        if (Process::current() == this && !Sys::CPU::current()->rescheduling)
            while (!Thread::current()->aborting())
                Thread::yield();
    }

    error_t Process::kill(pid_t pid, int sig) {
        IPC::siginfo_t info;

        info.si_signo = sig;
        info.si_uid   = Process::current()->real_uid;
        info.si_code  = SI_USER;

        return kill(pid, info);
    }

    error_t Process::kill(pid_t pid, IPC::siginfo_t info) {
        SCOPE(processes_lock);

        if (pid == -1) {
            if (info.si_signo == 0)
                return ENONE;

            auto process = process_list_head;

            for (int i = 0; i < process_list_size; i++) {
                if (inrange(process->pid, 0, 2) || process == Proc::Process::current()) {
                    process = process->next;
                    continue;
                }

                process->kill(info);
                process = process->next;
            }

            return ENONE;
        }

        if (pid < 0) {
            auto process = process_list_head;

            for (int i = 0; i < process_list_size; i++) {
                if (process->group != -pid) {
                    process = process->next;
                    continue;
                }

                process->kill(info);
                process = process->next;
            }

            return ENONE;
        }

        auto process = get_process(pid);
        if (!process || equals_one(process->status, TS_FRESH, TS_DEAD))
            return ESRCH;

        if (info.si_signo == 0)
            return ENONE;

        return process->kill(info);
    }

    error_t Process::kill(int sig) {
        IPC::siginfo_t info;

        info.si_signo = sig;
        info.si_uid   = Process::current()->real_uid;
        info.si_code  = SI_USER;

        return signal(info);
    }

    error_t Process::kill(IPC::siginfo_t info) {
        return signal(info);
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

            if (!val && val.error == ECHILD) {
                processes_lock.release();
                return val.error;
            }

            if (val) {
                printkd(PTKD_PROC, "proc: pid%i: Cleaned up pid%i\n", Process::current()->pid,
                        val.value.pid);

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

    O2 void Process::assoc(Thread* thread) {
        SCOPE(threads_lock);
        thread->tid = threads.push(thread);
    }

    O2 optional<Thread_SP> Process::unassoc(Thread* thread) {
        SCOPE(threads_lock);

        for (int i = 0; i < threads.count(); i++) {
            if (!threads.present(i) || threads[i] != thread)
                continue;

            auto thread_sp = threads[i];
            threads.erase(i);

            return thread_sp;
        }

        return ESRCH;
    }

    optional<Thread_SP> Process::get(tid_t tiddie) {
        SCOPE(threads_lock);
        if (!threads.present(tiddie))
            return ESRCH;

        return threads[tiddie];
    }

    Mem::Vector<char*, 4>& Process::env() {
        AEX_ASSERT(lock.isAcquired());
        return m_environment;
    }

    void Process::env(char* const envp[]) {
        clearEnv();
        SCOPE(lock);

        for (int i = 0; i < 256; i++) {
            if (!envp[i])
                break;

            char const* var   = envp[i];
            char*       var_d = new char[strlen(var) + 1];

            m_environment.push(var_d);
            strlcpy(var_d, var, strlen(var) + 1);
        }
    }

    void Process::env(Mem::Vector<char*, 4>* env) {
        clearEnv();
        SCOPE(lock);

        for (int i = 0; i < env->count(); i++) {
            char const* var   = env->at(i);
            char*       var_d = new char[strlen(var) + 1];

            m_environment.push(var_d);
            strlcpy(var_d, var, strlen(var) + 1);
        }
    }

    void Process::clearEnv() {
        SCOPE(lock);
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
            size_t len    = strlen(val);
            char*  buffer = new char[len + 1];

            m_environment.push(buffer);
            strlcpy(buffer, val, len + 1);

            return ENONE;
        }

        if (index < 0 || index >= m_environment.count())
            return EINVAL;

        size_t len = strlen(val);

        m_environment[index] = (char*) Mem::Heap::realloc(m_environment[index], len + 1);
        strlcpy(m_environment[index], val, len + 1);

        return ENONE;
    }

    void Process::purge_threads() {
        threads_lock.acquire();

        for (auto thread_try : threads) {
            if (!thread_try.has_value)
                continue;

            threads_lock.release();
            thread_try.value->abort(true);
            threads_lock.acquire();
        }

        threads_lock.release();

        while (Mem::atomic_read(&thread_counter) != 0)
            Proc::Thread::sleep(50);
    }

    void Process::purge_mmaps() {
        SCOPE(lock);

        for (int i = 0; i < mmap_regions.count(); i++) {
            if (!mmap_regions.present(i))
                continue;

            auto region = mmap_regions.at(i);
            mmap_regions.erase(i);

            delete region;
        }
    }

    void Process::purge_descriptors() {
        SCOPE(descs_mutex);

        for (int i = 0; i < descs.count(); i++) {
            if (!descs.present(i))
                continue;

            descs.erase(i);
        }
    }
}