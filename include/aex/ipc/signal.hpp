#pragma once

namespace AEX::IPC {
    enum signal_t {
        SIGABRT = 6,
        SIGALRM = 14,
        SIGFPE  = 8,
        SIGHUP  = 1,
        SIGILL  = 4,
        SIGINT  = 2,
        SIGKILL = 9,
        SIGPIPE = 13,
        SIGQUIT = 3,
        SIGSEGV = 11,
        SIGTERM = 15,
        SIGTRAP = 5,
    };
}