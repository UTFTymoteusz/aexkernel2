#include "aex/proc/process.hpp"

#include "aex/fs/path.hpp"
#include "aex/mem/heap.hpp"
#include "aex/string.hpp"

#include "proc/proc.hpp"

namespace AEX::Proc {
    Process::Process(const char* image_path, pid_t parent_pid, const char* name) {
        if (name == nullptr)
            FS::Path::get_filename(this->name, image_path, sizeof(this->name));
        else
            strncpy(this->name, name, sizeof(this->name));

        this->image_path = (char*) Heap::malloc(strlen(image_path) + 1);

        strncpy(this->image_path, image_path, strlen(image_path) + 1);

        this->parent_pid     = parent_pid;
        this->usage.cpu_time = 0;
        this->pid            = add_process(this);
    }
}