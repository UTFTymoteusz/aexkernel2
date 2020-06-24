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
        static Spinlock         _lock;
        static volatile int64_t _epoch;

        static bool _normal_hour_format;
        static bool _retarded_bcd;

        static void irq(void*);
    };
}