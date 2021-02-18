#include "aex/proc/exec.hpp"

#include "proc/proc.hpp"

namespace AEX::Proc {
    Mutex                  executor_mutex;
    Mem::Vector<Executor*> executors;

    void register_executor(Executor* executor) {
        auto scope = executor_mutex.scope();
        executors.push(executor);
    }

    error_t Executor::exec(Process*, Thread*, const char*, char* const[], char* const[]) {
        kpanic("Default AEX::Proc::Executor:exec() called");
    }

    error_t exec(Process* process, Thread* initiator, const char* path, char* const argv[],
                 char* const envp[], exec_opt* options) {
        if (process == Process::current()) {
            AEX_ASSERT(initiator->getProcess() == process);

            process->unassoc(initiator);

            auto thread = threaded_call(exec, process, initiator, path, argv, envp, options);

            thread->start();
            thread->join();

            return EINTR;
        }

        auto scope       = executor_mutex.scope();
        bool new_process = !process;

        if (new_process)
            process = new Process(path, Process::current()->pid,
                                  new Mem::Pagemap(0x00000000, 0x7FFFFFFFFFFF));

        for (int i = 0; i < executors.count(); i++) {
            if (executors[i]->exec(process, initiator, path, argv, envp))
                continue;

            process->rename(path, nullptr);
            if (process->get_cwd() == nullptr)
                process->set_cwd("/");

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

                auto desc = process->descs.at(i);
                if (desc.flags & FS::FD_CLOEXEC) {
                    desc.file->close();
                    process->descs.erase(i);
                }
            }

            if (options) {
                process->descs.set(0, options->stdin);
                process->descs.set(1, options->stdout);
                process->descs.set(2, options->stderr);
            }

            process->descs_lock.release();
            process->ready();

            if (initiator)
                initiator->abort(true);

            for (int i = 0; i < process->threads.count(); i++) {
                if (!process->threads.present(i))
                    continue;

                process->threads[i]->start();
            }

            return ENONE;
        }

        if (new_process) {
            processes_lock.acquire();
            delete process;
            processes_lock.release();
        }

        return ENOEXEC;
    }
}
