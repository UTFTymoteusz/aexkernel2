#pragma once

#include "aex/arch/sys/cpu.hpp"
#include "aex/utility.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX::Proc {
    struct API affinity {
        public:
        affinity() {
            m_mask = 0x00000000;
        }

        bool currentMasked() {
            return masked(Sys::CPU::currentID());
        }

        bool masked(int id) {
            return m_mask & (1 << id);
        }

        void mask(int id, bool mask) {
            if (mask)
                m_mask |= (1 << id);
            else
                m_mask &= ~(1 << id);
        }

        private:
        uint32_t m_mask;
    };
}