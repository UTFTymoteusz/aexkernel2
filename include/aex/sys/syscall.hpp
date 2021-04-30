#pragma once

#include "aex/utility.hpp"

namespace AEX::Sys {
    typedef void* syscall_t;

    /**
     * Gets the pointer to the default syscall table.
     * @returns The pointer to the default syscall table.
     **/
    API syscall_t* default_table();
}