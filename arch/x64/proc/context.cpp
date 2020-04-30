#include "proc/context.hpp"

#include "aex/mem/vmem.hpp"

namespace AEX::Proc {
    Context::Context(void* entry, void* stack, size_t stack_size, VMem::Pagemap* pagemap) {
        rip = (uint64_t) entry;
        rsp = (uint64_t) stack + stack_size;

        cr3 = (uint64_t) pagemap->pageRoot;

        cs = 0x08;
        ss = 0x10;

        rflags = 0x202;
    }
}