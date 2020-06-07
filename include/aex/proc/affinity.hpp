#pragma once

#include "aex/arch/sys/cpu.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX::Proc {
    struct affinity {
        public:
        affinity() {
            _mask = 0x00000000;
        }

        bool isCurrentCPUMasked() {
            return isMasked(Sys::CPU::getCurrentCPUID());
        }

        bool isMasked(int id) {
            return _mask & (1 << id);
        }

        void mask(int cpuid, bool mask) {
            if (mask)
                _mask |= (1 << cpuid);
            else
                _mask &= ~(1 << cpuid);
        }

        private:
        uint32_t _mask;
    };
}