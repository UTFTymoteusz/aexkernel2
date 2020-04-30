#include "proc/context.hpp"

#include "aex/mem/vmem.hpp"

namespace AEX::Proc {
    Context::Context(void* entry, void* stack, size_t stack_size, VMem::Pagemap* pagemap,
                     bool usermode) {
        rip = (uint64_t) entry;
        rsp = (uint64_t) stack + stack_size;

        cr3 = (uint64_t) pagemap->pageRoot;

        if (!usermode) {
            cs = 0x08;
            ss = 0x10;
        }
        else {
            cs = 0x23;
            ss = 0x1B;
        }

        rflags = 0x202;
    }
}