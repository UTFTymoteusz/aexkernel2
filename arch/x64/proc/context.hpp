#pragma once

#include <stddef.h>
#include <stdint.h>

namespace AEX::VMem {
    class Pagemap;
}

namespace AEX::Proc {
    class Context {
      public:
        uint64_t r15, r14, r13, r12, r11, r10, r9, r8, rbp, rdi, rsi, rdx, rcx, rbx, rax;
        uint64_t cr3;
        uint64_t rip, cs, rflags, rsp, ss;

        Context() = default;

        Context(void* entry, void* stack, size_t stack_size, VMem::Pagemap* pagemap,
                bool usermode = false, void (*on_end)() = nullptr);
    } __attribute__((packed));
}