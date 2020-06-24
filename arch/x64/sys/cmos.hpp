#pragma once

#include "aex/spinlock.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX::Sys {
    class CMOS {
        public:
        static constexpr auto STATUS_A = 0x0A;
        static constexpr auto STATUS_B = 0x0B;
        static constexpr auto STATUS_C = 0x0C;
        static constexpr auto STATUS_D = 0x0D;

        enum status_b_flags_t {
            STATUS_B_UPDATE_ENDED_INT = 1 << 4,
            STATUS_B_BINARY_MODE      = 1 << 2,
            STATUS_B_24_HOUR_MODE     = 1 << 1,
            STATUS_B_DST              = 1 << 0,
        };

        static uint8_t read(uint8_t addr);
        static void    write(uint8_t addr, uint8_t val);

        private:
        static Spinlock _lock;
    };
}