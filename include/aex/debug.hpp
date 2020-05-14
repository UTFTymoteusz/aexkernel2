#pragma once

namespace AEX::Debug {
    enum entry_type {
        BOOT   = 0,
        USER   = 1,
        KERNEL = 2,
    };

    void stack_trace(int skip = 0);
}