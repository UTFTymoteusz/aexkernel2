#include "aex/string.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX {
    void memcpy(void* dst, const void* src, size_t size) {
        size_t aligned = size / 8;

        asm volatile("mov rcx, %2; rep movsq;" : : "D"(dst), "S"(src), "r"(aligned));

        size_t remainder = size - aligned * 8;

        uint8_t* dst8 = (uint8_t*) dst + aligned * 8;
        uint8_t* src8 = (uint8_t*) src + aligned * 8;

        asm volatile("mov rcx, %2; rep movsb;" : : "D"(dst8), "S"(src8), "r"(remainder));
    }

    void memset(void* mem, char c, size_t len) {
        asm volatile("mov al, %1; mov rcx, %2; rep stosb;" : : "D"(mem), "r"(c), "r"(len));
    }

    void memset16(void* mem, uint16_t n, size_t count) {
        asm volatile("mov ax, %1; mov rcx, %2; rep stosd;" : : "D"(mem), "r"(n), "r"(count));
    }

    void memset32(void* mem, uint32_t n, size_t count) {
        asm volatile("mov eax, %1; mov rcx, %2; rep stosd;" : : "D"(mem), "r"(n), "r"(count));
    }

    void memset64(void* mem, uint64_t n, size_t count) {
        asm volatile("mov rax, %1; mov rcx, %2; rep stosq;" : : "D"(mem), "r"(n), "r"(count));
    }
}