#pragma once

#include "aex/spinlock.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX::Sys {
    class RTC {
        public:
        static void init();

        static int64_t get_epoch();

        private:
        static Spinlock         m_lock;
        static volatile int64_t m_epoch;

        static bool    m_normal_hour_format;
        static bool    m_retarded_bcd;
        static uint8_t m_century_index;

        static void irq(void*);
    };
}