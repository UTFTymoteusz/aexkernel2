#pragma once

#include <stddef.h>
#include <stdint.h>

namespace AEX::Debug {
    enum thread_entry_type {
        ENTRY_BOOT   = 0,
        ENTRY_USER   = 1,
        ENTRY_KERNEL = 2,
    };

    void stack_trace(int skip = 0);

    void load_kernel_symbols(const char* elf_path);

    const char* symbol_addr2name(void* addr, int* delta, bool only_kernel = false);
    const char* symbol_addr2name(void* addr, bool only_kernel = false);

    void* symbol_name2addr(const char* name);

    char* demangle_name(const char* symbol, char* buffer, size_t buffer_len);

    void dump_bytes(void* addr, size_t len);
}