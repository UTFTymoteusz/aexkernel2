#include "aex/string.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX {
    void memcpy(void* dst, const void* src, size_t size) {
        asm volatile("     \
            cld;           \
            \
            mov r8, rdi;   \
            mov r9, rsi;   \
            \
            mov rcx, rdx;  \
            shr rcx, 0x03; \
            \
            rep movsq;     \
            \
            mov rdi, r8;   \
            mov rsi, r9;   \
            \
            mov rcx, rdx;  \
            and rcx, 0x07; \
            and rdx, ~0x7; \
            \
            add rdi, rdx;  \
            add rsi, rdx;  \
            \
            rep movsb;     \
            "
                     :
                     : "D"(dst), "S"(src), "d"(size));
    }

    void memset(void* mem, char c, size_t len) {
        asm volatile("cld; mov al, %1; mov rcx, %2; rep stosb;" : : "D"(mem), "r"(c), "r"(len));
    }

    void memset16(void* mem, uint16_t n, size_t count) {
        asm volatile("cld; mov ax, %1; mov rcx, %2; rep stosd;" : : "D"(mem), "r"(n), "r"(count));
    }

    void memset32(void* mem, uint32_t n, size_t count) {
        asm volatile("cld; mov eax, %1; mov rcx, %2; rep stosd;" : : "D"(mem), "r"(n), "r"(count));
    }

    void memset64(void* mem, uint64_t n, size_t count) {
        asm volatile("cld; mov rax, %1; mov rcx, %2; rep stosq;" : : "D"(mem), "r"(n), "r"(count));
    }
}