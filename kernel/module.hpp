#pragma once

namespace AEX {
    void load_core_modules();

    const char* module_symbol_addr2name(void* addr, int* delta_ret);
}