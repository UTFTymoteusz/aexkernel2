#include "aex/proc/exec.hpp"

#include "proc/proc.hpp"

namespace AEX::Proc {
    Mutex                  executor_mutex;
    Mem::Vector<Executor*> executors;

    void registerExecutor(Executor* executor) {
        auto scope = executor_mutex.scope();

        executors.push(executor);
    }

    error_t exec(const char* path, exec_opt* options) {
        auto scope = executor_mutex.scope();

        auto process = new Process(path, Process::current()->pid,
                                   new Mem::Pagemap(0x00000000, 0x7FFFFFFFFFFF));

        for (int i = 0; i < executors.count(); i++) {
            if (executors[i]->exec(path, process))
                continue;

            process->files.set(0, options->stdin);
            process->files.set(1, options->stdout);
            process->files.set(2, options->stderr);

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
