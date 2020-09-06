#pragma once

#include "aex/module.hpp"

struct multiboot_info;
typedef struct multiboot_info multiboot_info_t;

namespace AEX {
    void load_symbols(multiboot_info_t* mbinfo);
    void load_modules(multiboot_info_t* mbinfo);
    void load_core_modules();

    error_t load_module(const char* path, bool block = false);
    error_t load_module(const char* label, void* addr, size_t len, bool block = false);

    const char* module_addr2name(void* addr, int& delta_ret);
    const char* module_addr2name(void* addr, int& delta_ret, Module*& module);

    void* module_name2addr_raw(const char* name, Module*& module);
}