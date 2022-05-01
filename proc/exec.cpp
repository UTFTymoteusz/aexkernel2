#include "aex/proc/exec.hpp"

#include "proc/proc.hpp"

namespace AEX::Proc {
    Mutex                  executor_mutex;
    Mem::Vector<Executor*> executors;

    void register_executor(Executor* executor) {
        SCOPE(executor_mutex);
        executors.push(executor);
    }

    error_t Executor::exec(Process*, Thread*, const char*, char* const[], char* const[]) {
        kpanic("Default AEX::Proc::Executor:exec() called");
    }

    error_t exec(Process* process, Thread* initiator, const char* path, char* const argv[],
                 char* const envp[], exec_opt* options) {
        if (process == Process::current()) {
            ASSERT(initiator->getProcess() == process);

            auto thread_sp = process->unassoc(initiator);
            thread_sp.value.defuse();

            auto thread = threaded_call(exec, process, initiator, path, argv, envp, options);

            thread->start();
            thread->join();

            return EINTR;
        }

        auto scope    = executor_mutex.scope();
        bool spawning = process == nullptr;

        if (spawning)
            process = new Process(path, Process::current()->pid,
                                  new Mem::Pagemap(0x00000000, 0x7FFFFFFFFFFF));

        for (int i = 0; i < executors.count(); i++) {
            if (executors[i]->exec(process, initiator, path, argv, envp))
                continue;

            process->rename(path, nullptr);
            if (process->get_cwd() == nullptr)
                process->set_cwd("/");

            using(process->lock) {
                for (int i = 0; i < process->mmap_regions.count(); i++) {
                    if (!process->mmap_regions.present(i))
                        continue;

                    auto region = process->mmap_regions.at(i);
                    process->mmap_regions.erase(i);

                    delete region;
                }
            }

            if (envp)
                process->env(envp);

            using(process->descs_mutex) {
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
            }

            process->ready();
            if (initiator)
                initiator->abort(true);

            using(process->threads_lock) {
                for (auto& thread_try : process->threads) {
                    if (!thread_try.has_value)
                        continue;

                    thread_try.value->start();
                }
            }

            return ENONE;
        }

        if (spawning) {
            using(processes_lock) {
                delete process;
            }
        }

        return ENOEXEC;
    }
}
