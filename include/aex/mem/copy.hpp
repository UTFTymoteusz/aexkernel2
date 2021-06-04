#pragma once

#include "aex/string.hpp"

namespace AEX::Mem {
    error_t a2k_memcpy(void* dst, const void* src, size_t len) {
        // use a setjmp to handle exceptions

        memcpy(dst, src, len);
        return ENONE;
    }

    error_t k2a_memcpy(void* dst, const void* src, size_t len) {
        // use a setjmp to handle exceptions

        memcpy(dst, src, len);
        return ENONE;
    }
}