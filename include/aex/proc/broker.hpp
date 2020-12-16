#pragma once

#include <stddef.h>

/**
 * The AEX broker is a simple thread in a loop which executes whatever function you request it to
 * execute. It is useful if you need to run something in a foreign thread and don't want to create a
 * whole new thread.
 **/

namespace AEX::Proc {
    /**
     * Requests the broker to execute a function.
     * @param func The function to execute.
     * @param arg  The first and only argument of the function.
     **/
    void broker(void* (*func)(void* arg), void* arg);

    /**
     * Requests the broker to execute a function.
     * @param func The function to execute.
     * @param arg  The first and only argument of the function.
     **/
    template <typename AT>
    void broker(void (*func)(AT arg), AT arg) {
        broker((void* (*) (void*) )(size_t) func, (void*) arg);
    }

    /**
     * Requests the broker to execute a function.
     * @param func The function to execute.
     **/
    inline void broker(void (*func)()) {
        broker((void* (*) (void*) )(size_t) func, nullptr);
    }
}
