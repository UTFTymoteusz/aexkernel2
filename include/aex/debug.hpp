#pragma once

#include "aex/utility.hpp"

#include <stddef.h>
#include <stdint.h>

#define COOKIE() ((AEX::Debug::cookie){__FILE__, __LINE__})
#define COOKIEB() ((AEX::Debug::cookie){__FILE__, 0})

namespace AEX::Debug {
    enum thread_entry_type {
        ENTRY_BOOT   = 0,
        ENTRY_USER   = 1,
        ENTRY_KERNEL = 2,
    };

    struct cookie {
        const char* file;
        int         line;
    };

    struct stack_frame;

    extern bool symbols_loaded;
    extern bool flag;

    API void stack_trace(int skip = 0, stack_frame* frame = nullptr);

    API const char* addr2name(void* addr, int& delta, bool only_kernel = false);
    API const char* addr2name(void* addr, bool only_kernel = false);
    API void*       name2addr(const char* name);

    API char* demangle_name(const char* symbol, char* buffer, size_t buffer_len);

    API void dump_bytes(void* addr, size_t len);

    void load_symbols(const char* elf_path);
    void load_symbols(void* addr);
}