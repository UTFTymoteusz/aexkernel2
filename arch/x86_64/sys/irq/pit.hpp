#pragma once

#include "aex/spinlock.hpp"

#include <stdint.h>

namespace AEX::Sys::IRQ {
    class PIT {
        public:
        static void hz(int hz);
        static void interval(double ms);

        static void interrupt(double ms);

        private:
        static Spinlock m_lock;
    };
}