#pragma once

#include "aex/dev/tty/tty.hpp"
#include "aex/mem.hpp"
#include "aex/spinlock.hpp"
#include "aex/utility.hpp"

namespace AEX::Dev::Input {
    void init();
    void tty_loop();
}

namespace AEX::Dev::TTY {
    constexpr auto ROOT_TTY   = 0;
    constexpr auto TTY_AMOUNT = 1;

    class TTY;
    class VTTY;

    /**
     * An array of all terminals.
     **/
    API extern TTY* TTYs[TTY_AMOUNT];

    /**
     * An array of all virtual terminals.
     **/
    API extern VTTY* VTTYs[TTY_AMOUNT];

    /**
     * Initializes the bare neccesities required for a single terminal.
     **/
    void init(multiboot_info_t* mbinfo);

    /**
     * Initializes all terminals properly.
     **/
    void init_mem(multiboot_info_t* mbinfo);
}