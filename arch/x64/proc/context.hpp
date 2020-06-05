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

        uint64_t padding;

        uint8_t fxstate[512];

        Context() = default;

        Context(void* entry, void* stack, size_t stack_size, VMem::Pagemap* pagemap,
                bool usermode = false, void (*on_exit)() = nullptr);

        template <typename T>
        void setArguments(T a) {
            rdi = (uint64_t) a;
        }

        template <typename T, typename T2>
        void setArguments(T a, T2 b) {
            rdi = (uint64_t) a;
            rsi = (uint64_t) b;
        }

        template <typename T, typename T2, typename T3>
        void setArguments(T a, T2 b, T3 c) {
            rdi = (uint64_t) a;
            rsi = (uint64_t) b;
            rdx = (uint64_t) c;
        }

        template <typename T, typename T2, typename T3, typename T4>
        void setArguments(T a, T2 b, T3 c, T4 d) {
            rdi = (uint64_t) a;
            rsi = (uint64_t) b;
            rdx = (uint64_t) c;
            rcx = (uint64_t) d;
        }
    } __attribute__((packed));
}