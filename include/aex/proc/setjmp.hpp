#pragma once

#include "aex/arch/proc/context.hpp"

namespace AEX::Proc {
    extern "C" {
    int               setjmp(Context* context);
    [[noreturn]] void longjmp(Context* context, int status);
    bool              testjmp(Context* context);
    void              nojmp(Context* context);
    }
}
