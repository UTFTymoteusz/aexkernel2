#pragma once

#include "sys/cpu.hpp"

#include <stdint.h>

namespace AEX::Proc {
    class Affinity {
      public:
        Affinity() {
            _mask = 0xFFFFFFFF;
        }

        bool canRun() {
            return canRun(Sys::CPU::getCurrentCPUID());
        }

        bool canRun(int cpuid) {
            return _mask & (1 << cpuid);
        }

      private:
        uint32_t _mask;
    };
}