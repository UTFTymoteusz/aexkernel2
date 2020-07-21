#pragma once

namespace AEX {
    void load_core_modules();

    error_t load_module(const char* path);
    error_t load_module_from_memory(void* addr, size_t len, const char* path);

    const char* module_symbol_addr2name(void* addr, int* delta_ret);
}