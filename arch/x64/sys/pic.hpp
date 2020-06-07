#pragma once

#include <stddef.h>
#include <stdint.h>

namespace AEX::Sys {
    class PIC {
        public:
        int command;
        int data;

        PIC() = default;
        PIC(int cmd, int dat) {
            command = cmd;
            data    = dat;
        }

        void init(uint8_t start, bool slave = false);
        void setMask(uint8_t mask);
    };

    extern PIC pics[2];

    constexpr PIC* MASTER_PIC = &pics[0];
    constexpr PIC* SLAVE_PIC  = &pics[1];
}