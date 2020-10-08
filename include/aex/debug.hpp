#pragma once

#include <stddef.h>
#include <stdint.h>

namespace AEX::Debug {
    enum thread_entry_type {
        ENTRY_BOOT   = 0,
        ENTRY_USER   = 1,
        ENTRY_KERNEL = 2,
    };

    extern bool symbols_loaded;
    extern bool flag;

    void stack_trace(int skip = 0);

    void load_symbols(const char* elf_path);
    void load_symbols(void* addr);

    const char* addr2name(void* addr, int& delta, bool only_kernel = false);
    const char* addr2name(void* addr, bool only_kernel = false);

    void* name2addr(const char* name);

    char* demangle_name(const char* symbol, char* buffer, size_t buffer_len);

    void dump_bytes(void* addr, size_t len);

    void symbol_debug();
}