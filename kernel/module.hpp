#pragma once

#include "aex/module.hpp"

#include "boot/mboot.h"

namespace AEX {
    void load_multiboot_symbols(multiboot_info_t* mbinfo);
    void load_multiboot_modules(multiboot_info_t* mbinfo);
    void load_core_modules();

    error_t load_module(const char* path, bool block = false);
    error_t load_module_from_memory(void* addr, size_t len, const char* path, bool block = false);

    const char* module_symbol_addr2name(void* addr, int& delta_ret);
    const char* module_symbol_addr2name(void* addr, int& delta_ret, Module*& module);

    void* module_symbol_name2addr_raw(const char* name, Module*& module);
}