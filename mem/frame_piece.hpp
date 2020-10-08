#pragma once

#include "aex/assert.hpp"
#include "aex/kpanic.hpp"
#include "aex/mem.hpp"
#include "aex/string.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX::Mem::Phys {
    struct frame_piece {
        phys_addr start;

        uint32_t     size;
        uint32_t     frames_free;
        frame_piece* next;

        frame_piece(phys_addr addr, uint32_t amnt) {
            init(addr, amnt);
        }

        void init(phys_addr addr, uint32_t amnt) {
            start = addr;

            size        = amnt;
            frames_free = amnt;

            next = nullptr;

            memset(m_bitmap, 0, amnt / 8);
        }

        void alloc(int32_t lid, uint32_t amount) {
            uint32_t ii = lid / (sizeof(uint32_t) * 8);
            uint16_t ib = lid % (sizeof(uint32_t) * 8);

            uint32_t tmp;

            for (size_t i = 0; i < amount; i++) {
                tmp = 1 << ib;

                AEX_ASSERT(!(m_bitmap[ii] & tmp));

                m_bitmap[ii] |= tmp;

                ib++;
                if (ib >= sizeof(uint32_t) * 8) {
                    ii++;
                    ib = 0;
                }
            }
            frames_free -= amount;
        }

        void free(int32_t lid, uint32_t amount) {
            uint32_t ii = lid / (sizeof(uint32_t) * 8);
            uint16_t ib = lid % (sizeof(uint32_t) * 8);

            uint32_t tmp;

            for (size_t i = 0; i < amount; i++) {
                tmp = 1 << ib;

                AEX_ASSERT(m_bitmap[ii] & tmp);

                m_bitmap[ii] &= ~tmp;

                ib++;
                if (ib >= sizeof(uint32_t) * 8) {
                    ii++;
                    ib = 0;
                }
            }
            frames_free += amount;
        }

        int32_t find(uint32_t amount) {
            if (amount == 0 || amount > frames_free)
                return -1;

            uint32_t ii = 0;
            uint16_t ib = 0;

            uint32_t found = 0;
            int32_t  start = -1;

            uint32_t tmp;

            for (size_t i = 0; i < size; i++) {
                tmp = 1 << ib;

                if (!(m_bitmap[ii] & tmp)) {
                    if (start == -1)
                        start = i;

                    found++;
                    if (found == amount)
                        return start;
                }
                else {
                    found = 0;
                    start = -1;
                }

                ib++;
                if (ib >= sizeof(uint32_t) * 8) {
                    ii++;
                    ib = 0;
                }
            }
            return -1;
        }

        uint32_t m_bitmap[];
    };
}