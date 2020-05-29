#pragma once

#include <stddef.h>

namespace AEX::Debug {
    enum entry_type {
        BOOT   = 0,
        USER   = 1,
        KERNEL = 2,
    };

    void stack_trace(int skip = 0);

    void load_kernel_symbols(const char* elf_path);

    const char* symbol_addr2name(void* addr);
    void*       symbol_name2addr(const char* name);

    char* demangle_name(const char* symbol, char* buffer, size_t buffer_len);

    void dumb_bytes(void* addr, size_t len);
}