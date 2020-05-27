#include "proc/context.hpp"

#include "aex/debug.hpp"
#include "aex/mem/vmem.hpp"

namespace AEX::Proc {
    Context::Context(void* entry, void* stack, size_t stack_size, VMem::Pagemap* pagemap,
                     bool usermode, void (*on_exit)()) {
        rip = (uint64_t) entry;
        rsp = (uint64_t) stack + stack_size;

        cr3 = (uint64_t) pagemap->pageRoot;

        rbp = 0;

        if (on_exit) {
            rsp -= sizeof(uint64_t);
            *((uint64_t*) rsp) = usermode ? Debug::entry_type::USER : Debug::entry_type::KERNEL;

            rsp -= sizeof(on_exit);
            *((uint64_t*) rsp) = (uint64_t) on_exit;
        }

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