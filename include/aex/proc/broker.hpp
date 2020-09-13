#pragma once

namespace AEX::Proc {
    void broker(void* (*func)(void* arg), void* arg);

    template <typename AT>
    void broker(void (*func)(AT arg), AT arg) {
        broker((void* (*) (void*) ) func, (void*) arg);
    }
}
