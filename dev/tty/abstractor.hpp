#pragma once

#include "aex/dev/tty/tty.hpp"
#include "aex/dev/tty/vtty.hpp"

namespace AEX::Dev::TTY {
    class Abstractor : public TTY {
        public:
        Abstractor();
        Abstractor(VTTY* vtty);

        char read();

        TTY& write(char c);
        TTY& write(const char* str);

        bool text();
        bool graphics();

        tty_info info();
        void*    output();

        private:
        enum state {
            OUT       = 0,
            ANSI_CAT  = 1,
            ANSI_ARGS = 2,
            ANSI_CODE = 3,
        };

        VTTY* m_vtty;
        state m_state;

        int m_arg = 0;
        int m_ptr = 0;

        int  m_args[8];
        char m_cat;
        char m_code;

        char m_buffer[32];

        void _write(char c);
        void flush_ansi();
    };
}
