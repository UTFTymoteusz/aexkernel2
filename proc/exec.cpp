#include "aex/proc/exec.hpp"

#include "proc/proc.hpp"

namespace AEX::Proc {
    Mutex                  executor_mutex;
    Mem::Vector<Executor*> executors;

    void registerExecutor(Executor* executor) {
        auto scope = executor_mutex.scope();

        executors.push(executor);
    }

    error_t exec(const char* path) {
        auto scope = executor_mutex.scope();

        auto process = new Process(path, Process::current()->pid,
                                   new Mem::Pagemap(0x00000000, 0x7FFFFFFFFFFF));

        for (int i = 0; i < executors.count(); i++) {
            if (executors[i]->exec(path, process))
                continue;

            process->ready();

            return ENONE;
        }

        processes_lock.acquire();
        delete process;
        processes_lock.release();

        return ENOEXEC;
    }
}
