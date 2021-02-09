#pragma once

namespace AEX::Sys {
    class CPU;
}

namespace AEX::Sys::MCore {
    extern int   cpu_count;
    extern CPU** CPUs;

    /**
     * Starts all CPUs in the system.
     **/
    void init();
}