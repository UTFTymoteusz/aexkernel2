#include "aex/proc/exec.hpp"

#include "proc/proc.hpp"

namespace AEX::Proc {
    Mutex                  executor_mutex;
    Mem::Vector<Executor*> executors;

    void register_executor(Executor* executor) {
        auto scope = executor_mutex.scope();

        executors.push(executor);
    }

    error_t exec(Process* process, Thread* initiator, const char* path, char* const argv[],
                 char* const envp[], exec_opt* options) {
        if (process == Process::current()) {
            auto thread = threaded_call(exec, process, initiator, path, argv, envp, options);

            thread->start();
            thread->detach();

            while (!Thread::current()->aborting())
                Thread::sleep(5);

            return EINTR;
        }

        auto scope = executor_mutex.scope();

        if (!process)
            process = new Process(path, Process::current()->pid,
                                  new Mem::Pagemap(0x00000000, 0x7FFFFFFFFFFF));

        for (int i = 0; i < executors.count(); i++) {
            if (executors[i]->exec(process, initiator, path, argv, envp))
                continue;

            process->rename(path, nullptr);
            if (process->get_cwd() == nullptr)
                process->set_cwd("/");

            process->files_lock.acquire();

            if (options) {
                process->files.set(0, options->stdin);
                process->files.set(1, options->stdout);
                process->files.set(2, options->stderr);
            }

            process->files_lock.release();
            process->ready();

            for (int i = 0; i < process->threads.count(); i++) {
                if (!process->threads.present(i))
                    continue;

                process->threads[i]->start();
            }

            return ENONE;
        }

        processes_lock.acquire();
        delete process;
        processes_lock.release();

        return ENOEXEC;
    }
}
