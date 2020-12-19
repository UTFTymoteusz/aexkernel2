#include "aex/proc/process.hpp"

#include "aex/assert.hpp"
#include "aex/fs.hpp"
#include "aex/mem.hpp"
#include "aex/proc.hpp"
#include "aex/proc/broker.hpp"
#include "aex/string.hpp"
#include "aex/sys/syscall.hpp"

#include "proc/proc.hpp"

namespace AEX::Proc {
    Process::Process(const char* image_path, pid_t parent_pid, Mem::Pagemap* pagemap,
                     const char* name) {
        rename(image_path, name);

        processes_lock.acquire();

        this->parent_pid        = parent_pid;
        this->usage.cpu_time_ns = 0;
        this->pid               = add_process(this);
        this->pagemap           = pagemap;
        this->syscall_table     = Sys::default_table();

        processes_lock.release();
    }

    Process::~Process() {
        if (pagemap != Mem::kernel_pagemap)
            delete pagemap;

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
        return process_list_head->next;
    }

    void exit_threaded(Process* process) {
        for (int i = 0; i < process->threads.count(); i++) {
            if (!process->threads.present(i))
                continue;

            process->threads.at(i)->abort();
        }

        while (Mem::atomic_read(&process->thread_counter) != 0)
            Proc::Thread::sleep(50);

        processes_lock.acquire();
        process->status = TS_DEAD;

        auto iter_process = process_list_head;
        for (int i = 0; i < process_list_size; i++) {
            if (iter_process->parent_pid == process->pid)
                iter_process->parent_pid = process->parent_pid;

            iter_process = iter_process->next;
        }

        get_process(process->parent_pid)->child_event.raise();
        PRINTK_DEBUG1("pid%i: full exit", process->pid);

        processes_lock.release();
    }

    void exit_threaded_broker(Process* process) {
        auto thread_try = threaded_call(exit_threaded, process);
        AEX_ASSERT(thread_try);

        thread_try->detach();
    }

    void Process::exit(int status) {
        auto scope = processes_lock.scope();

        if (m_exiting)
            return;

        m_exiting = true;
        ret_code  = status;

        PRINTK_DEBUG2("pid%i: exit(%i)", pid, status);

        broker(exit_threaded_broker, this);

        if (Process::current() == this) {
            processes_lock.release();

            while (!Thread::current()->interrupted())
                Thread::yield();

            processes_lock.acquire();
        }
    }

    error_t Process::kill(pid_t pid) {
        auto scope   = processes_lock.scope();
        auto process = get_process(pid);
        if (!process)
            return ESRCH;

        if (process->status == TS_FRESH)
            return ESRCH;

        process->exit(0);
        return ENONE;
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
        auto    scope   = processes_lock.scope();
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
        auto scope = Process::current()->lock.scope();

        while (true) {
            auto val = try_get(Process::current()->pid);
            if (!val && val.error_code == ECHILD)
                return val.error_code;

            if (val) {
                PRINTK_DEBUG2("pid%i: cleaned up pid%i", Process::current()->pid, val.value.pid);

                status = val.value.code;
                return val.value.pid;
            }

            Process::current()->child_event.wait();
            Process::current()->lock.release();

            Thread::yield();

            Process::current()->lock.acquire();
            if (Thread::current()->interrupted())
                return EINTR;
        }
    }
}