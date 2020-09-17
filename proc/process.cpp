#include "aex/proc/process.hpp"

#include "aex/fs.hpp"
#include "aex/mem.hpp"
#include "aex/proc.hpp"
#include "aex/string.hpp"

#include "proc/proc.hpp"

namespace AEX::Proc {
    Process::Process(const char* image_path, pid_t parent_pid, Mem::Pagemap* pagemap,
                     const char* name) {
        if (name == nullptr)
            FS::Path::get_filename(this->name, image_path, sizeof(this->name));
        else
            strncpy(this->name, name, sizeof(this->name));

        this->image_path = new char[strlen(image_path) + 1];

        strncpy(this->image_path, image_path, strlen(image_path) + 1);

        this->parent_pid        = parent_pid;
        this->usage.cpu_time_ns = 0;
        this->pid               = add_process(this);
        this->pagemap           = pagemap;
    }

    Process::~Process() {
        //
    }

    Process* Process::current() {
        return Thread::current()->parent;
    }

    void Process::exit(int status) {
        auto scope = processes_lock.scope();

        PRINTK_DEBUG2("pid%i: exit(%i)", pid, status);
        // delete this;
    }
}