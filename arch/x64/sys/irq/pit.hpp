#pragma once

#include "aex/spinlock.hpp"

#include <stdint.h>

namespace AEX::Sys::IRQ {
    class PIT {
        public:
        static void setHz(int hz);
        static void setInterval(double ms);

        static void interruptIn(double ms);

        private:
        static Spinlock m_lock;
    };
}