#pragma once

#include "aex/proc/process.hpp"
#include "aex/proc/thread.hpp"

#include <stddef.h>

namespace AEX::Mem {
    class usr_stack {
        public:
        usr_stack(Proc::Thread* thread, size_t rsp) {
            m_thread = thread;

            m_min = thread->user_stack;
            m_max = thread->user_stack + thread->user_stack_size;

            m_rsp = rsp;
        }

        ~usr_stack() {
            if (m_mapped)
                kernel_pagemap->free(m_mapped, 4096);
        }

        size_t pointer() {
            return m_rsp;
        }

        int push_bytes(uint8_t* bytes, int len) {
            for (int i = 0; i < len; i++)
                if (!push_byte(bytes[i]))
                    return i;

            return len;
        }

        int push_bytes_inv(uint8_t* bytes, int len) {
            for (int i = 0; i < len; i++)
                if (!push_byte(bytes[len - 1 - i]))
                    return i;

            return len;
        }

        bool push_byte(uint8_t byte) {
            m_rsp--;

            uint64_t new_page = m_rsp / 4096;
            if (new_page != m_page) {
                m_mapped = (uint8_t*) kernel_pagemap->map(
                    4096, m_thread->getProcess()->pagemap->paddrof((void*) (new_page * 4096)), 0);
                m_page = new_page;
            }

            m_mapped[m_rsp - new_page * 4096] = byte;

            return true;
        }

        template <typename T>
        int push(T val) {
            return push_bytes_inv((uint8_t*) &val, sizeof(val));
        }

        private:
        Proc::Thread* m_thread;

        size_t m_min, m_max;
        size_t m_rsp;

        uint64_t m_page   = 0xFFFFFFFFFFFF;
        uint8_t* m_mapped = nullptr;
    };
}