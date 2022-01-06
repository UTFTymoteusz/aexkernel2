#pragma once

#include "aex/assert.hpp"
#include "aex/kpanic.hpp"
#include "aex/mem.hpp"
#include "aex/string.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX::Mem::Phys {
    typedef uint32_t bitmap_t;

    struct frame_piece {
        phys_t start;

        uint32_t     size;
        uint32_t     frames_free;
        frame_piece* next;

        bitmap_t m_bitmap[];

        frame_piece(phys_t addr, uint32_t amnt) {
            init(addr, amnt);
        }

        void init(phys_t addr, uint32_t amnt) {
            start       = addr;
            size        = amnt;
            frames_free = amnt;
            next        = nullptr;

            memset(m_bitmap, 0, amnt / 8);
        }

        void alloc(int32_t lid, uint32_t amount) {
            uint32_t ii            = lid / (sizeof(bitmap_t) * 8);
            uint16_t ib            = lid % (sizeof(bitmap_t) * 8);
            uint32_t original_amnt = amount;

            if (ib != 0) {
                int bit_count = clamp(amount, 0u, 32u - ib);
                for (int i = 0; i < bit_count; i++)
                    m_bitmap[ii] |= 1 << ib++;

                amount -= bit_count;

                ii++;
            }

            for (size_t i = 0; i < amount / 32; i++)
                m_bitmap[ii++] = 0xFFFFFFFF;

            amount -= int_floor(amount, 32u);

            for (size_t i = 0; i < amount; i++)
                m_bitmap[ii] |= 1 << i;

            frames_free -= original_amnt;
        }

        void free(int32_t lid, uint32_t amount) {
            uint32_t ii            = lid / (sizeof(bitmap_t) * 8);
            uint16_t ib            = lid % (sizeof(bitmap_t) * 8);
            uint32_t original_amnt = amount;
            uint32_t tmp           = 0x00;

            if (ib != 0) {
                int bit_count = clamp(amount, 0u, 32u - ib);
                for (int i = 0; i < bit_count; i++)
                    tmp |= 1 << ib++;

                m_bitmap[ii++] &= ~tmp;

                amount -= bit_count;
            }

            for (size_t i = 0; i < amount / 32; i++)
                m_bitmap[ii++] = 0x00000000;

            amount -= int_floor(amount, 32u);
            tmp = 0x00;

            for (size_t i = 0; i < amount; i++)
                tmp |= 1 << i;

            m_bitmap[ii] &= ~tmp;
            frames_free += original_amnt;
        }

        int32_t find(uint32_t amount) {
            if (amount == 0 || amount > frames_free)
                return -1;

            uint32_t ii    = 0;
            uint16_t ib    = 0;
            uint32_t found = 0;
            int32_t  start = -1;

            for (size_t i = 0; i < size; i++) {
                if (ib == 0 && m_bitmap[ii] == 0x00000000) {
                    if (start == -1)
                        start = i;

                    found += 32;
                    if (found >= amount)
                        return start;

                    ii++;
                    i += 32;

                    continue;
                }

                if (!(m_bitmap[ii] & (1 << ib))) {
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
                if (ib >= sizeof(bitmap_t) * 8) {
                    ii++;
                    ib = 0;
                }
            }
            return -1;
        }
    };
}