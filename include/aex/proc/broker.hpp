#pragma once

#include <stddef.h>

namespace AEX::Proc {
    void broker(void* (*func)(void* arg), void* arg);

    template <typename AT>
    void broker(void (*func)(AT arg), AT arg) {
        broker((void* (*) (void*) )(size_t) func, (void*) arg);
    }
}
