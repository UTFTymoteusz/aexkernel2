#pragma once

#include "aex/arch/proc/context.hpp"
#include "aex/mem/paging.hpp"
#include "aex/optional.hpp"
#include "aex/utility.hpp"

#include <stddef.h>

namespace AEX::Mem {
    /**
     * Userspace stack manipulator class.
     **/
    class API Stacker {
        public:
        Stacker(size_t rsp) {
            m_rsp = rsp;
        }

        size_t pointer() {
            return m_rsp;
        }

        size_t push_bytes(const uint8_t* bytes, size_t len) {
            if (m_rsp < len)
                len = m_rsp;

            m_rsp -= len;
            memcpy((void*) m_rsp, bytes, len);

            return len;
        }

        template <typename T>
        bool push(T val) {
            return push_bytes((uint8_t*) &val, sizeof(val)) == sizeof(val);
        }

        size_t pop_bytes(uint8_t* bytes, size_t len) {
            if (Mem::kernel_pagemap->vstart - m_rsp < len)
                len = Mem::kernel_pagemap->vstart - m_rsp;

            memcpy(bytes, (void*) m_rsp, len);
            m_rsp += len;

            return len;
        }

        template <typename T>
        optional<T> pop() {
            T val;
            if (pop_bytes((uint8_t*) &val, sizeof(val)) != sizeof(T))
                return {};

            return val;
        }

        void move(int amnt) {
            m_rsp -= amnt;
        };

        private:
        virt_t m_rsp;
    };
}