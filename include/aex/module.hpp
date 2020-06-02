#pragma once

#include "aex/errno.hpp"

namespace AEX {
    error_t load_module(const char* path);

    void  register_global_symbol(const char* name, void* addr);
    void* get_global_symbol(const char* name);
}